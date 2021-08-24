import my_config

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
        logger.debug(f'args, kwargs: {args, kwargs}')

        return_value = func(*args, **kwargs)

        logger.debug(f"{func.__name__} returned '{return_value}'")
        logger.debug("========================================================")

        return return_value

    return wrapper


class Actions(object):
    @debugmethod
    def make_map(self, input, start, end, elements):
        # Skip opening curly brace
        d = elements[1]
        # More than one pair is optional. Walk through any remaining pairs and
        # merge them into the dictionary.
        for el in elements[2]:
            d = {**d, **el}
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
    def struct_top(self, input, start, end, elements):
        return elements[2].text + elements[3].text

    @debugmethod
    def make_struct(self, input, start, end, elements):
        d = {}
        for el in elements[2]:
            if el is not None:
                d = {**d, **el}
        return { elements[1] : d }

my_config_example = '''
struct test1
    key1 = "value"
    key2 = 1.342
    key3 = 10
    f = "none"
end test1

struct test2
    my_key = "foo"
    n_key = 1
end test2
'''

logger.setLevel(logging.DEBUG)
result = my_config.parse(my_config_example, actions=Actions())
pprint.pprint(result)
        
