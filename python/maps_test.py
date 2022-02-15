import maps


import functools
import json
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
            d = {**d, **el.PAIR}
        return d

    @debugmethod
    def make_pair(self, input, start, end, elements):
        return {elements[0]: elements[2]}

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

print("---------------------- Test 1 -------------------------------------------------------------")
logger.setLevel(logging.DEBUG)
result = maps.parse('{"ints":[1, 2,  -3 ], "more_ints": [0001, 0002, -05]}', actions=Actions())
print(result)

print("---------------------- Test 2 -------------------------------------------------------------")
logger.setLevel(logging.INFO)
result = maps.parse('{"ints"  :   "test"     }', actions=Actions())
print(result)

print("---------------------- Test 3 -------------------------------------------------------------")
logger.setLevel(logging.INFO)
json_simple = '{"id":"0001","type":0.55,"thing"  :[1, 2.2 ,5.  ], "sci_not_test": [1e3, 1.e-5, -4.3e+5] }'
result2 = maps.parse(json_simple, actions=Actions())
print(result2)


print("---------------------- Test 4 -------------------------------------------------------------")
logger.setLevel(logging.DEBUG)
json_example = '''{
	"id": "0001",
	"type": "donut",
	"name": "Cake",
	"ppu": 0.55,
	"batters": [ 0.2, 0.4, 0.6 ] }'''
result = maps.parse(json_example, actions=Actions())
print(result)

print("---------------------- Test 5 -------------------------------------------------------------")
logger.setLevel(logging.INFO)
json_example = '''{
	"id": "0001",
	"type": "donut",
	"name": "Cake",
	"ppu": 0.55,
	"batters":
		{
			"batter":
				[
					{ "id": "1001", "type": "Regular" },
					{ "id": "1002", "type": "Chocolate" },
					{ "id": "1003", "type": "Blueberry" },
					{ "id": "1004", "type": "Devil's Food" }
				]
		},
	"topping":
		[
			{ "id": "5001", "type": "None" },
			{ "id": "5002", "type": "Glazed" },
			{ "id": "5005", "type": "Sugar" },
			{ "id": "5007", "type": "Powdered Sugar" },
			{ "id": "5006", "type": "Chocolate with Sprinkles" },
			{ "id": "5003", "type": "Chocolate" },
			{ "id": "5004", "type": "Maple" }
		]
}'''

result2 = maps.parse(json_example, actions=Actions())
pprint.pprint(result2)

print("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
pprint.pprint(json.loads(json_example))

assert(result2 == json.loads(json_example))
