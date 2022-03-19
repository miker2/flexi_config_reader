import collections
import copy
import pprint

import pe

from helpers import *

class ProtoVar:
    def __init__(self, name, value=None):
        self.name = name
        self.value = value

    def __repr__(self):
        return f"${self.name}={self.value}"

class Proto(collections.abc.Mapping):
    def __init__(self, name, proto):
        self.name = name
        self.proto = proto

    def __repr__(self):
        return f"!PROTO! {self.name} : {self.proto}"

    def __getitem__(self, key):
        if key == self.name:
            return self.proto
        else:
            raise KeyError(f"'{key}' is not a valid key")

    def __len__(self):
        return 1

    def __iter__(self):
        yield self.name

class ReferenceVar:
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __repr__(self):
        return f"${self.name}={self.value}"

class Reference(collections.abc.Mapping):
    def __init__(self, name, proto, content):
        self.name = name
        self.proto = proto
        self.content = content

    def __repr__(self):
        return f"!REFERENCE! {self.proto} as {self.name} : {self.content}"

    def __getitem__(self, key):
        if key == self.name:
            return {'proto': self.proto, 'content': self.content}
        else:
            raise KeyError(f"'{key}' is not a valid key")

    def __len__(self):
        return 1

    def __iter__(self):
        yield self.name

class ValueLookup:
    def __init__(self, name):
        self.keys = name.split('.')

    @property
    def var(self):
        return '.'.join(self.keys)

    def __repr__(self):
        return f"$({self.var})"


@debugmethod
def make_map(*elements, **kwargs):
    # Skip opening whitespace
    d = []
    # More than one pair is optional. Walk through any remaining pairs and
    # merge them into the dictionary.
    for el in elements:
        d.append(el)
        #d = {**d, **el}
    return d

@debugmethod
def make_pair(*elements, **kwargs):
    return {elements[0]: elements[1]}

@debugmethod
def found_key(s):
    return s

@debugmethod
def make_list(*elements, **kwargs):
    return list(elements)

@debugmethod
def make_number(v):
    # Try creating an 'int' from the string first. If the string contains a float, then an
    # exception will be raised. Fail back to creating a float.
    try:
        return int(v, 10)
    except ValueError:
        return float(v)

@debugmethod
def make_var(elements):
    return ProtoVar(elements[1:])

@debugmethod
def ref_sub_var(*elements, **kwargs):
    return ReferenceVar(elements[0].name, elements[1])

@debugmethod
def ref_add_var(*elements, **kwargs):
    return {elements[0] : elements[1]}

@debugmethod
def var_ref(*elements, **kwargs):
    return ValueLookup(elements[0][2:-1])

@debugmethod
def make_struct(*elements, **kwargs):
    assert( elements[0] == elements[-1] )
    d = {}
    for el in elements[1:-1]:
        if isinstance(el, Reference) or isinstance(el, Proto):
            d[el.name] = el
        elif el is not None:
            d = {**d, **el}
    return { elements[0] : d }

@debugmethod
def make_proto(*elements, **kwargs):
    assert( elements[0] == elements[-1] )
    d = {}
    for el in elements[1:-1]:
        if isinstance(el, Reference):
            d[el.name] = el
        elif el is not None:
            d = {**d, **el}
    return { elements[0] : Proto(elements[0], d) }

@debugmethod
def make_reference(*elements, **kwargs):
    assert( elements[1] == elements[-1] )
    logger.debug(f"proto: {elements[0]}")
    logger.debug(f"struct: {elements[1]}")
    l = []
    d = {}
    for el in elements[2:-1]:
        print(el)
        if el is None:
            continue
        if isinstance(el, dict):
            d = {**d, **el}
        else:
            l.append(el)
    l.append(d)
    logger.debug(f"contents: {l}")
    return Reference(elements[1], elements[0], l)


# TODO: Figure out how to delete empty structs/dicts efficiently (for example, when protos are
# removed from the struct, they may leave an empty parent.

class ConfigReader:
    def __init__(self, cfg_str, verbose=False):
        self.cfg_str = cfg_str
        if verbose:
            logger.setLevel(logging.DEBUG)
        else:
            logger.setLevel(logging.INFO)

        self._grammar = None
        with open("my_config.peg", 'r') as f:
            self._grammar = f.read()

        self._parser = pe.compile(self._grammar, flags=pe.OPTIMIZE,
                                  actions={'map' : make_map,
                                           'struct' : make_struct,
                                           'proto' : make_proto,
                                           'reference' : make_reference,
                                           'PAIR' : make_pair,
                                           'REF_VARSUB' : ref_sub_var,
                                           'REF_VARADD' : ref_add_var,
                                           'VAR_REF': pe.actions.Capture(var_ref),
                                           'VAR' : pe.actions.Capture(make_var),
                                           'KEY' : pe.actions.Capture(found_key),
                                           'FLAT_KEY' : pe.actions.Capture(found_key),
                                           'STRING' : pe.actions.Capture(lambda x: x[1:-1]),
                                           'HEX' : pe.actions.Capture(lambda x: int(x, 16)),
                                           'NUMBER' : pe.actions.Capture(make_number),
                                           'list' : make_list})

        out = self._parser.match(self.cfg_str)
        print(out)
        print(out.groups())
        self._parse_result = self._parser.match(self.cfg_str).value()
        print("Parsing completed successfully! Result:")
        pprint.pprint(self._parse_result)

        self._protos = {}
        self.cfg = self._resolve_output()

    @staticmethod
    def parse_from_file(filename, verbose=False):
        with open(filename) as f:
            cfg = ConfigReader(f.read(), verbose)

        return cfg

    def _resolve_output(self):
        ''' Resolves the output of the PEG parsed data into a fully qualified struct containing
            all of the config entries. '''
        # First, we need to flatten all of the data in the list
        logger.info("----- Flatten list into dict ----------------")
        flat_data = {}
        for el in self._parse_result:
            if isinstance(el, dict):
                flat_data = {**flat_data, **self._flatten(el)}
            else:
                flat_data[el.name] = el
            logger.info(f" *** For {el}: \n +++ flat_data: {flat_data}")

        logger.info("flat_data:")
        logger.info(pprint.pformat(flat_data))

        logger.info("----- Find all protos ----------------")
        # This is going to be inefficient, but we need to walk the list and create a list of all protos
        self._protos = { k : v for k, v in flat_data.items() if isinstance(v, Proto) }

        logger.info(pprint.pformat(self._protos))

        logger.info("----- Merge nested dictionaries ----------------")
        d = {}
        for el in self._parse_result:
            d = merge_nested_dict(d, el)

        logger.info(pprint.pformat(d))

        logger.info("----- Remove protos from dictionary ----------------")
        # Remove the protos from merged dictionary
        for p in self._protos.keys():
            parts = p.split('.')
            tmp = d
            for i in range(len(parts)-1):
                tmp = tmp[parts[i]]
            del tmp[parts[-1]]

        logger.info(pprint.pformat(d))

        logger.info("----- Resolve all references  ----------------")
        self._resolve_references(d)
        logger.info(pprint.pformat(d))

        logger.info("----- Resolving variable references ----------")
        self._resolve_var_ref(flat_data, d, d)
        logger.info(pprint.pformat(d))

        logger.info("----- Done! ----------------")
        return d



    #@debugmethod
    def _flatten(self, d, name=None, flat=None):
        if flat is None:
            flat = {}
        for k, v in d.items():
            new_name = make_name(name, k)
            if isinstance(v, dict):
                flat = self._flatten(v, name=new_name, flat=flat)
            else:
                flat[new_name] = v
                print(new_name)
        return flat


    @debugmethod
    def _replace_proto_var(self, d, ref_var):
        for k, v in d.items():
            if isinstance(v, ProtoVar):
                d[k] = ref_var[v.name]
            elif isinstance(v, str):
                # Find instances of 'ref_var' in 'v' and replace.
                for rk, rv in ref_var.items():
                    print(f"v: {v}, rk: ${rk}, rv: {rv}")
                    v = v.replace(f"${rk}", rv)
                    print(f"v: {v}, rk: ${{{rk}}}, rv: {rv}")
                    v = v.replace(f"${{{rk}}}", rv)
                    print(f"v: {v}")
                d[k] = v
            elif isinstance(v, dict):
                self._replace_proto_var(v, ref_var)


    @debugmethod
    def _resolve_references(self, d, name=None, ref_vars=None):
        if ref_vars is None:
            ref_vars = {}
        for k, v in d.items():
            new_name = make_name(name, k)
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
                        r = merge_nested_dict(r, el)

                print(f"Ref vars: {ref_vars}")
                self._replace_proto_var(r, ref_vars)
                d[k] = r
                # Call recursively in case the current reference has another reference
                self._resolve_references(r, make_name(new_name, k), ref_vars)

            elif isinstance(v, dict):
                self._resolve_references(v, new_name, ref_vars)

    @debugmethod
    def _resolve_var_ref(self, flat_data, root, d):
        for k, v in d.items():
            if isinstance(v, ValueLookup):
                try:
                    value = flat_data[v.var]
                except KeyError:
                    value = get_dict_value(root, v.keys)
                print(f"Found instance of ValueLookup: {v}. Has value={value}")
                d[k] = value
            elif isinstance(v, dict):
                self._resolve_var_ref(flat_data, root, v)
