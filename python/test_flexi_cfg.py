import math
import os
import unittest
import json

import flexi_cfg


class TestMyConfig(unittest.TestCase):
    def setUp(self):
        pass

    def test_cfg_string(self):
        my_config_example = '''
        struct test1 {
          key1 = "value"
          key2 = 1.342    # test comment here
          key3 = 10
          f = "none"
        }

        # Does this comment work here?

        struct test2 {
          # And another comment.
          my_key = "foo"
          n_key = 1
          bool_key = false
          my_hex   =   0x4539
          var_ref = $(test1.key3)
        }

        my_list = [1.0, -2, 4.2]
        '''

        expected_cfg = {'test1': {'f': 'none',
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10},
                        'test2': {'my_key': 'foo',
                                  'n_key': 1,
                                  'my_hex': 0x4539,
                                  'bool_key': False,
                                  'var_ref': 10},
                        'my_list': [1.0, -2, 4.2]}
        cfg = flexi_cfg.parse_from_string(my_config_example, "example_cfg")
        self.assertEqual(cfg.get_string("test1.key1"), expected_cfg["test1"]["key1"])
        self.assertEqual(cfg.get_float("test1.key2"), expected_cfg["test1"]["key2"])
        self.assertEqual(cfg.get_int("test1.key3"), expected_cfg["test1"]["key3"])
        self.assertEqual(cfg.get_uint64("test2.my_hex"), expected_cfg["test2"]["my_hex"])
        self.assertEqual(cfg.get_bool("test2.bool_key"), expected_cfg["test2"]["bool_key"])
        self.assertEqual(cfg.get_float_list("my_list"), expected_cfg['my_list'])

        self.assertEqual(sorted(cfg.keys()), sorted(list(expected_cfg.keys())))
        self.assertEqual(cfg.get_type("test1.key1"), flexi_cfg.Type.STRING)
        self.assertEqual(cfg.get_type("test1.key2"), flexi_cfg.Type.NUMBER)
        self.assertEqual(cfg.get_type("test1.key3"), flexi_cfg.Type.NUMBER)
        self.assertEqual(cfg.get_type("test2.my_hex"), flexi_cfg.Type.NUMBER)
        self.assertEqual(cfg.get_type("test2.bool_key"), flexi_cfg.Type.BOOLEAN)
        self.assertEqual(cfg.get_type("my_list"), flexi_cfg.Type.LIST)

        cfg_test2 = cfg.get_reader("test2")
        self.assertEqual(sorted(cfg_test2.keys()), sorted(expected_cfg["test2"].keys()))

    def parse_cfg_file(self):
        cfg_file = "config_example1.cfg"
        try:
            cfg_file_path = os.path.join(os.environ['EXAMPLES_DIR'], cfg_file)
        except KeyError:
            cfg_file_path = os.path.join("../examples", cfg_file)
        return flexi_cfg.parse(cfg_file_path)

    def expected_cfg_file(self):
        expected_cfg = {'another_key': "test",
                        'test1': {'f': ['foo', 'bar', 'baz'],
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 0.5 * (-0.25 * math.pi)},
                        'test2': {'my_key': 'foo',
                                  'n_key': 0x1234,
                                  'var_ref': None},
                        'q': {'e': 2},
                        'solo_key': 10.1,
                        'int_list': [0, 1, 3, -5],
                        'uint_list': [0x0, 0x1, 0x3, 0x5],
                        'float_list': [0.0, 1.0, 3.0, -5.0],
                        'uint64': -5,
                        'a' : 2,
                        'b' : 2,
                        'c' : 2,
                        'd' : 2,}
        expected_cfg['test2']['var_ref'] = expected_cfg['test1']['key3']
        return expected_cfg;

    def test_cfg_file(self):
        cfg = self.parse_cfg_file()
        expected_cfg = self.expected_cfg_file()

        self.assertEqual(sorted(cfg.keys()), sorted(list(expected_cfg.keys())))
        self.assertEqual(cfg.get_float('test1.key3'), cfg.get_float('test2.var_ref'))
        self.assertAlmostEqual(cfg.get_float('test1.key3'), expected_cfg['test1']['key3'], places=6)

        self.assertEqual(cfg.get_type('test1.f'), flexi_cfg.Type.LIST)
        self.assertEqual(cfg.get_string_list('test1.f'), expected_cfg['test1']['f'])

        # Check that parsing a signed integer as an unsigned integer produces an exception.
        excepted = False
        try:
            cfg.get_uint64('uint64')
        except flexi_cfg.MismatchTypeException:
            excepted = True
        except:
            self.assertTrue(False)
        self.assertTrue(excepted)

        # Check various list types:
        self.assertEqual(cfg.get_int_list('int_list'), expected_cfg['int_list'])
        self.assertEqual(cfg.get_uint64_list('uint_list'), expected_cfg['uint_list'])
        self.assertEqual(cfg.get_float_list('float_list'), expected_cfg['float_list'])

        # Check the generic get_value() method:
        self.assertEqual(cfg.get_value('test1.key1'), expected_cfg['test1']['key1'])
        self.assertEqual(cfg.get_value('test1.key2'), expected_cfg['test1']['key2'])
        self.assertAlmostEqual(cfg.get_value('test1.key3'), expected_cfg['test1']['key3'], places=6)
        self.assertEqual(cfg.get_value('test2.my_key'), expected_cfg['test2']['my_key'])
        self.assertEqual(cfg.get_value('test2.n_key'), expected_cfg['test2']['n_key'])
        self.assertAlmostEqual(cfg.get_value('test2.var_ref'), expected_cfg['test2']['var_ref'], places=6)
        self.assertEqual(cfg.get_value('solo_key'), expected_cfg['solo_key'])
        self.assertEqual(cfg.get_value('int_list'), expected_cfg['int_list'])
        self.assertEqual(cfg.get_value('uint_list'), expected_cfg['uint_list'])
        self.assertEqual(cfg.get_value('float_list'), expected_cfg['float_list'])
        self.assertEqual(cfg.get_value('uint64'), expected_cfg['uint64'])

    def validate_cfg_file(self, cfg):
        expected_cfg = self.expected_cfg_file()
        # deep equals check on config dicts
        self.assertEqual(cfg, expected_cfg, "JSON parsed config should match the Python dict")

    def test_cfg_file_as_json(self):
        cfg_reader = self.parse_cfg_file()
        cfg = json.loads(cfg_reader.json())
        self.validate_cfg_file(cfg)

    def test_cfg_file_as_json(self):
        cfg_reader = self.parse_cfg_file()
        cfg = json.loads(cfg_reader.json(pretty=False))
        self.validate_cfg_file(cfg)

if __name__ == '__main__':
    unittest.main(verbosity=2)
