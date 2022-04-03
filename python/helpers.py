import functools
import logging
import pprint

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
        logger.debug(f'args: {pprint.pformat(args)}')
        logger.debug(f'kwargs: {kwargs}')

        return_value = func(*args, **kwargs)

        logger.debug(f"{func.__name__} returned '{return_value}'")
        logger.debug("========================================================")

        return return_value

    return wrapper

class DuplicateKeyException(Exception):
    def __init__(self, key, dict_a, dict_b):
        Exception.__init__(self, f"Found duplicate key: '{key}'!")

        self.key = key
        self.dict_a = dict_a
        self.dict_b = dict_b

class MismatchKeyException(Exception):
    def __init__(self, key, dict_a, dict_b):
        Exception.__init__(self, f"Mismatched types for key: '{key}'!")

        self.key = key
        self.dict_a = dict_a
        self.dict_b = dict_b

def merge_nested_dict(dict1, dict2, allow_overwrite=False):
    ''' Merge dictionaries recursively and keep all nested keys combined between
        the two dictionaries. Any key/value pairs that already exist in the
        leaves of dict1 will be overwritten by the same key/value pairs from
        dict2.'''

    def check_for_errors(key):
        ''' Three cases to check for:
              - Both are dictionaries     - This is okay
              - Neither are dictionaries  - This is bad - even if the same value, we don't allow duplicates
              - Only ones is a dictionary - Also bad. We can't handle this one
        '''
        dict_count = isinstance(dict1[key], dict) + isinstance(dict2[key], dict)
        if dict_count == 0:    # Neither is a dictionary. We don't support duplicate keys
            raise DuplicateKeyException(key, dict1, dict2)
            # print(f"Found duplicate key: '{key}' with values '{dict1[key]}' and '{dict2[key]}'!!!")
        elif dict_count == 1:  # One dictionary, but not the other, can't merge these
            raise MismatchKeyException(key, dict1, dict2)

        return not (dict_count == 2)  # All good if both are dictionaries (for now)

    common_keys = set(dict1.keys()).intersection(dict2.keys())
    #print(f"Common keys: {common_keys}")
    for k in common_keys:
        if not allow_overwrite:
            check_for_errors(k)

    # This over-writes the top level keys in dict1 with dict2. If there are
    # nested dictionaries, we need to handle these appropriately.
    out_dict = {**dict1, **dict2}
    for key, value in out_dict.items():
        if key in dict1 and key in dict2:
            # Find any values in the top-level keys that are also dictionaries.
            # Call this function recursively.
            if type(dict1[key]) == dict and type(dict2[key]) == dict:
                out_dict[key] = merge_nested_dict(dict1[key], dict2[key], allow_overwrite)

    return out_dict

# Using an array of nested keys, lookup the value from a dictionary
@debugmethod
def get_dict_value(d, keys, ko=None):
    if ko is None:
        ko = keys

    k0 = keys[0]
    keys = keys[1:]
    v = d[k0]

    if len(keys) == 0:
        return v
    elif isinstance(v, dict):
        return get_dict_value(v, keys, ko)
    else:
        # The resulting value isn't a dictionary, and there are more keys in the list. This is
        # an error!
        raise Exception(f"key list contains {len(keys)} elements, but exhausted dictionary search. "
                        f"Original key={'.'.join(ko)}, Remaining keys={'.'.join(keys)}")


def make_name(n1, n2=None):
    # Check that at least one argument is valid. We could just return an empty string, but that
    # seems silly.
    if (not n1 or len(n1) == 0) and (not n2 or len(n2) == 0):
        raise ValueError("At least one argument must be non-empty")
    
    if not n1 or len(n1) == 0:
        return n2
    elif not n2 or len(n2) == 0:
        return n1

    return f"{n1}.{n2}"
