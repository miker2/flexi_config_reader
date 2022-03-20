import unittest

import helpers
from helpers import DuplicateKeyException, MismatchKeyException

# Disable most log messages during testing.
helpers.ch.setLevel(helpers.logging.WARNING)


class TestMergeNestedDict(unittest.TestCase):
    def test_merge_nested_dict_simple(self):
        dict_a = {'test1' : {'key1' : 1, 'key2' : 2}}
        dict_b = {'test2' : {'key1' : 1, 'key2' : 2}}
        actual = helpers.merge_nested_dict(dict_a, dict_b)
        expected = {'test1' : {'key1' : 1, 'key2' : 2},
                    'test2' : {'key1' : 1, 'key2' : 2}}
        self.assertEqual(actual, expected)

    def test_merge_nested_dict_nested(self):
        dict_a = {'test1' : {'key1' : 1, 'key2' : 2, 'test2' : {'key3' : 3}}}
        dict_b = {'test1' : {'test2' : {'key1' : 1, 'key2' : 2}, 'key3' : 3},
                  'test2' : {'key1' : 1, 'key2' : 2}}
        actual = helpers.merge_nested_dict(dict_a, dict_b)
        expected = {'test1' : {'key1' : 1,
                               'key2' : 2,
                               'key3' : 3,
                               'test2' : {'key1' : 1,
                                          'key2' : 2,
                                          'key3' : 3}},
                    'test2' : {'key1' : 1,
                               'key2' : 2}}
        self.assertEqual(actual, expected)

    def test_merge_nested_dict_duplicate(self):
        dict_a = {'test1' : {'key1' : 1, 'key2' : 2}}
        dict_b = {'test1' : {'key1' : 5, 'key3' : 2}}
        duplicate_key = 'key1'
        with self.assertRaises(DuplicateKeyException) as exception_context:
            actual = helpers.merge_nested_dict(dict_a, dict_b)
        self.assertEqual(
            str(exception_context.exception),
            f"Found duplicate key: '{duplicate_key}'!")
        self.assertEqual(exception_context.exception.key, duplicate_key)

    def test_merge_nested_dict_mismatch(self):
        dict_a = {'test1' : {'key1' : 1, 'key2' : 2}}
        dict_b = {'test1' : None}
        mismatch_key = 'test1'
        with self.assertRaises(MismatchKeyException) as exception_context:
            actual = helpers.merge_nested_dict(dict_a, dict_b)
        self.assertEqual(
            str(exception_context.exception),
            f"Mismatched types for key: '{mismatch_key}'!")
        self.assertEqual(exception_context.exception.key, mismatch_key)


class TestGetDictValue(unittest.TestCase):
    def test_get_dict_value_nested(self):
        test_dictionary = {'test1' : {'key1' : 1,
                                      'key2' : 2,
                                      'key3' : 3,
                                      'test2' : {'key1' : 1,
                                                 'key2' : 2,
                                                 'key3' : 3}},
                           'test2' : {'key1' : 1,
                                      'key2' : 2}}
        value = helpers.get_dict_value(test_dictionary, ['test1', 'test2', 'key2'])
        expected_value = test_dictionary['test1']['test2']['key2']
        self.assertEqual(value, expected_value)

    def test_get_dict_value_simple(self):
        test_dictionary = {'key1' : 1,
                           'key2' : 2,
                           'key3' : 3}
        for k,v in test_dictionary.items():
            value = helpers.get_dict_value(test_dictionary, [k])
            expected_value = v
            self.assertEqual(value, expected_value)

    def test_get_dict_value_too_many_keys(self):
        test_dictionary = {'test1' : {'test2' : None }}
        test_keys = ['test1', 'test2', 'test3']
        with self.assertRaises(Exception) as exception_context:
            actual = helpers.get_dict_value(test_dictionary, test_keys)
        self.assertTrue(isinstance(exception_context.exception, Exception))
        self.assertTrue(str(exception_context.exception).startswith(
            "key list contains 1 elements, but exhausted dictionary search."))

    def test_get_dict_value_not_found(self):
        test_dictionary = {'test1' : {'test2' : None }}
        with self.assertRaises(KeyError) as exception_context:
            actual = helpers.get_dict_value(test_dictionary, ['test1', 'test3'])
            self.assertEqual(str(exception_context.exception), "test3")


class TestMakeName(unittest.TestCase):
    def test_make_name_first(self):
        n1 = "name_1"
        self.assertEqual(helpers.make_name(n1, None), n1)
        self.assertEqual(helpers.make_name(n1, ""), n1)
        self.assertEqual(helpers.make_name(n1), n1)

    def test_make_name_last(self):
        n2 = "name_2"
        self.assertEqual(helpers.make_name(None, n2), n2)
        self.assertEqual(helpers.make_name("", n2), n2)

    def test_make_name_both(self):
        n1 = "name_1"
        n2 = "name_2"
        self.assertEqual(helpers.make_name(n1, n2), f"{n1}.{n2}")

    def test_make_name_neiher(self):
        self.assertRaises(ValueError, helpers.make_name, None, None)
        self.assertRaises(ValueError, helpers.make_name, None, "")
        self.assertRaises(ValueError, helpers.make_name, "", None)
        self.assertRaises(ValueError, helpers.make_name, "", "")
        

if __name__ == '__main__':
    # suite_merge_nested_dict = unittest.TestLoader().loadTestsFromTestCase(TestMergeNestedDict)
    # suite_get_dict_value = unittest.TestLoader().loadTestsFromTestCase(TestGetDictValue)
    # suite_make_name = unittest.TestLoader().loadTestsFromTestCase(TestMakeName)
    # suite = unittest.TestSuite([suite_merge_nested_dict, suite_get_dict_value, suite_make_name])
    # result = unittest.TestResult()
    # suite.run(result)

    # print(result)
    
    unittest.main(verbosity=2)
