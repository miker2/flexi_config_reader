import my_config

import copy
import functools
import logging
import pprint

import ipdb

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
# create formatter
formatter = logging.Formatter('%(message)s')

# add formatter to ch
ch.setFormatter(formatter)
logger.addHandler(ch)


def debugmethod(func):
    """ Debug a method and return it back"""

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        logger.debug("++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        logger.debug(f'Calling : {func.__name__}')
        logger.debug(f'args: {args}')
        logger.debug(f'kwargs: {kwargs}')

        return_value = func(*args, **kwargs)

        logger.debug(f"{func.__name__} returned '{return_value}'")
        logger.debug("========================================================")

        return return_value

    return wrapper


class ProtoVar(object):
    def __init__(self, name, value=None):
        self.name = name
        self.value = value

    def __repr__(self):
        return f"${self.name}={self.value}"

class Proto(object):
    def __init__(self, name, proto):
        self.name = name
        self.proto = proto

    def __repr__(self):
        return f"!PROTO! {self.name} : {self.proto}"

class ReferenceVar(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __repr__(self):
        return f"${self.name}={self.value}"

class Reference(object):
    def __init__(self, name, proto, content):
        self.name = name
        self.proto = proto
        self.content = content

    def __repr__(self):
        return f"!REFERENCE! {self.proto} as {self.name} : {self.content}"

# TODO: Add merging of nested structs. Currently, duplicate structs get overwritten, which is not
#       what we want.
# NOTE: We also don't want to allow duplicate values, so we need to handle those gracefully as well.
class Actions(object):
    @debugmethod
    def make_map(self, input, start, end, elements):
        print(elements[1])
        # Skip opening whitespace
        d = []
        # More than one pair is optional. Walk through any remaining pairs and
        # merge them into the dictionary.
        for el in elements[1]:
            d.append(el)
            #d = {**d, **el}
        return d

    @debugmethod
    def make_pair(self, input, start, end, elements):
        return {elements[0]: elements[2]}

    @debugmethod
    def found_key(self, input, start, end, elements):
        key = elements[0].text
        key += elements[1].text
        return key

    @debugmethod
    def make_string(self, input, start, end, elements):
        return elements[1].text

    @debugmethod
    def make_list(self, input, start, end, elements):
        l = [elements[1]]
        for el in elements[2]:
            l.append(el.value)
        return l

    @debugmethod
    def mak_hex(self, input, start, end, elements):
        return int(input[start:end], base=0)

    @debugmethod
    def make_number(self, input, start, end, elements):
        # Try creating an 'int' from the string first. If the string contains a float, then
        # creating an 'int' will fail. Fall back to a float.
        try:
            return int(input[start:end], 10)
        except ValueError:
            return float(input[start:end])

    @debugmethod
    def make_var(self, input, start, end, elements):
        return ProtoVar(elements[1].text)

    @debugmethod
    def ref_sub_var(self, input, start, end, elements):
        return ReferenceVar(elements[0].name, elements[2])

    @debugmethod
    def ref_add_var(self, input, start, end, elements):
        return {elements[1] : elements[3]}

    @debugmethod
    def make_struct(self, input, start, end, elements):
        assert( elements[1] == elements[4] )
        d = {}
        for el in elements[2]:
            if isinstance(el, Reference) or isinstance(el, Proto):
                d[el.name] = el
            elif el is not None:
                d = {**d, **el}
        return { elements[1] : d }

    @debugmethod
    def make_proto(self, input, start, end, elements):
        assert( elements[1] == elements[4] )
        d = {}
        for el in elements[2]:
            if isinstance(el, Reference):
                d[el.name] = el
            elif el is not None:
                d = {**d, **el}
        #return Proto(elements[1], d)
        return { elements[1] : Proto(elements[1], d) }

    @debugmethod
    def make_reference(self, input, start, end, elements):
        assert( elements[4] == elements[7] )
        logger.debug(f"proto: {elements[1]}")
        logger.debug(f"struct: {elements[4]}")
        l = []
        d = {}
        for el in elements[5]:
            if el is None:
                continue
            if isinstance(el, dict):
                d = {**d, **el}
            else:
                l.append(el)
        l.append(d)
        logger.debug(f"contents: {l}")
        return Reference(elements[4], elements[1], l)


######### HELPERS ########################################################################
def _merge_nested_dict(dict1, dict2, allow_overwrite=False):
    ''' Merge dictionaries recursively and keep all nested keys combined between
        the two dictionaries. Any key/value pairs that already exist in the
        leaves of dict1 will be overwritten by the same key/value pairs from
        dict2.'''

    def checkForErrors(key):
        ''' Three cases to check for:
              - Both are dictionaries     - This is okay
              - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
              - Only ones is a dictionary - Also bad. We can't handle this one
        '''
        dict_count = isinstance(dict1[key], dict) + isinstance(dict2[key], dict)
        if dict_count == 0:    # Neither is a dictionary. We don't support duplicate keys
            print(f"Found duplicate key: '{key}' with values '{dict1[key]}' and '{dict2[key]}'!!!")
        elif dict_count == 1:  # One dictionary, but not the other, can't merge these
            print(f"Mismatched types for key '{key}'! One is a dict, the other is a value")

        return dict_count != 2  # All good if both are dictionaries (for now)

    common_keys = set(dict1.keys()).intersection(dict2.keys())
    #print(f"Common keys: {common_keys}")
    for k in common_keys:
        assert(allow_overwrite or not checkForErrors(k))

    # This over-writes the top level keys in dict1 with dict2. If there are
    # nested dictionaries, we need to handle these appropriately.
    out_dict = {**dict1, **dict2}
    for key, value in out_dict.items():
        if key in dict1 and key in dict2:
            # Find any values in the top-level keys that are also dictionaries.
            # Call this function recursively.
            if type(dict1[key]) == dict and type(dict2[key]) == dict:
                out_dict[key] = _merge_nested_dict(dict1[key], dict2[key], allow_overwrite)

    return out_dict


def makeName(n1, n2):
    if not n1 or len(n1) == 0:
        return n2
    elif not n2 or len(n2) == 0:
        return n1

    return f"{n1}.{n2}"





class ConfigReader:
    def __init__(self, cfg_str, verbose=False):
        self.cfg_str = cfg_str
        if verbose:
            logger.setLevel(logging.DEBUG)
        else:
            logger.setLevel(logging.INFO)
        
        self._parse_result = my_config.parse(self.cfg_str, actions=Actions())
        print("Parsing completed successfully! Result:")
        pprint.pprint(self._parse_result)

        self._protos = {}
        self.cfg = self._resolve_output()

    @staticmethod
    def parse_from_file(filename):
        with open(filename) as f:
            cfg = ConfigReader(f.read())

        return cfg
    
    def _resolve_output(self):
        ''' Resolves the output of the PEG parsed data into a fully qualified struct containing
            all of the config entries. '''
        # First, we need to flatten all of the data in the list
        print("----- Flatten list into dict ----------------")
        flat_data = {}
        for el in self._parse_result:
            if isinstance(el, dict):
                flat_data = {**flat_data, **self._flatten(el, l={})}
            else:
                flat_data[el.name] = el
            print(f" *** For {el}: \n +++ flat_data: {flat_data}")

        print("flat_data:")
        pprint.pprint(flat_data)

        print("----- Find all protos ----------------")
        # This is going to be inefficient, but we need to walk the list and create a list of all protos
        self._protos = { k : v for k, v in flat_data.items() if isinstance(v, Proto) }

        pprint.pprint(self._protos)

        print("----- Merge nested dictionaries ----------------")
        d = {}
        for el in self._parse_result:
            d = _merge_nested_dict(d, el)

        pprint.pprint(d)
    
        print("----- Remove protos from dictionary ----------------")
        # Remove the protos from merged dictionary
        for p in self._protos.keys():
            parts = p.split('.')
            tmp = d
            for i in range(len(parts)-1):
                tmp = tmp[parts[i]]
            del tmp[parts[-1]]

        pprint.pprint(d)

        print("----- Resolve all references  ----------------")
        self._resolve_references(d)
        pprint.pprint(d)

        print("----- Done! ----------------")
        return d



    #@debugmethod
    def _flatten(self, d, name=None, l={}):
        for k, v in d.items():
            new_name = makeName(name, k)
            if isinstance(v, dict):
                l = self._flatten(v, name=new_name, l=l)
            else:
                l[new_name] = v
                print(new_name)
        return l


    @debugmethod
    def _replace_proto_var(self, d, ref_var):
        for k, v in d.items():
            if isinstance(v, ProtoVar):
                d[k] = ref_var[v.name]
            elif isinstance(v, dict):
                self._replace_proto_var(v, ref_var)


    @debugmethod
    def _resolve_references(self, d, name=None, ref_vars={}):
        for k, v in d.items():
            new_name = makeName(name, k)
            if isinstance(v, Reference):
                p = self._protos[v.proto]
                print(f"p: {p}")
                r = copy.deepcopy(p.proto)
                print(f"r: {v}")
                for el in v.content:
                    if isinstance(el, ReferenceVar):
                        # fill this in
                        print(f"Sub var: {el}")
                        ref_vars[el.name] = el.value
                    elif isinstance(el, dict):
                        # append to proto
                        print(f"new keys: {el}")
                        r = _merge_nested_dict(r, el)

                print(f"Ref vars: {ref_vars}")
                self._replace_proto_var(r, ref_vars)
                d[k] = r
                # Call recursively in case the current reference has another reference
                self._resolve_references(r, makeName(new_name, k), ref_vars)

            elif isinstance(v, dict):
                self._resolve_references(v, new_name)
