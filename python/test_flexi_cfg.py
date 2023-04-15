import logging
import math
import os
import pprint
import unittest

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
        cfg = flexi_cfg.parse(my_config_example, "example_cfg")
        self.assertEqual(cfg.getString("test1.key1"), expected_cfg["test1"]["key1"])
        self.assertEqual(cfg.getFloat("test1.key2"), expected_cfg["test1"]["key2"])
        self.assertEqual(cfg.getInt("test1.key3"), expected_cfg["test1"]["key3"])
        self.assertEqual(cfg.getUint64("test2.my_hex"), expected_cfg["test2"]["my_hex"])
        self.assertEqual(cfg.getBool("test2.bool_key"), expected_cfg["test2"]["bool_key"])
        self.assertEqual(cfg.getFloatList("my_list"), expected_cfg['my_list'])

        self.assertEqual(sorted(cfg.keys()), sorted(expected_cfg.keys()))
        self.assertEqual(cfg.getType("test1.key1"), flexi_cfg.Type.kString)
        self.assertEqual(cfg.getType("test1.key2"), flexi_cfg.Type.kNumber)
        self.assertEqual(cfg.getType("test1.key3"), flexi_cfg.Type.kNumber)
        self.assertEqual(cfg.getType("test2.my_hex"), flexi_cfg.Type.kNumber)
        self.assertEqual(cfg.getType("test2.bool_key"), flexi_cfg.Type.kBoolean)
        self.assertEqual(cfg.getType("my_list"), flexi_cfg.Type.kList)

        cfg_test2 = cfg.getReader("test2")
        self.assertEqual(sorted(cfg_test2.keys()), sorted(expected_cfg["test2"].keys()))
        
    def test_cfg_file(self):
        cfg_file = "config_example1.cfg"
        try:
            cfg_file_path = os.path.join(os.environ['EXAMPLES_DIR'], cfg_file)
        except KeyError:
            cfg_file_path = os.path.join("../examples", cfg_file)

        expected_cfg = {'another_key': "test",
                        'test1': {'f': ['foo', 'bar', 'baz'],
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 0.5 * (-0.25 * math.pi)},
                        'test2': {'my_key': 'foo',
                                  'n_key': 0x1234,
                                  'var_ref': None},
                        'solo_key': 10.1,
                        'int_list': [0, 1, 3, -5],
                        'uint_list': [0x0, 0x1, 0x3, 0x5],
                        'float_list': [0.0, 1.0, 3.0, -5.0],
                        'uint64': -5}
        expected_cfg['test2']['var_ref'] = expected_cfg['test1']['key3']
        
        cfg = flexi_cfg.parse(cfg_file_path)

        self.assertEqual(sorted(cfg.keys()), sorted(expected_cfg.keys()))
        self.assertEqual(cfg.getFloat('test1.key3'), cfg.getFloat('test2.var_ref'))
        self.assertAlmostEqual(cfg.getFloat('test1.key3'), expected_cfg['test1']['key3'], places=6)

        self.assertEqual(cfg.getType('test1.f'), flexi_cfg.Type.kList)
        self.assertEqual(cfg.getStringList('test1.f'), expected_cfg['test1']['f'])

        # Check that parsing a signed integer as an unsigned integer produces an exception.
        excepted = False
        try:
            cfg.getUint64('uint64')
        except flexi_cfg.MismatchTypeException:
            excepted = True
        except:
            self.assertTrue(False)
        self.assertTrue(excepted)

        # Check various list types:
        self.assertEqual(cfg.getIntList('int_list'), expected_cfg['int_list'])
        self.assertEqual(cfg.getUint64List('uint_list'), expected_cfg['uint_list'])
        self.assertEqual(cfg.getFloatList('float_list'), expected_cfg['float_list'])

        # Check the generic getValue() method:
        self.assertEqual(cfg.getValue('test1.key1'), expected_cfg['test1']['key1'])
        self.assertEqual(cfg.getValue('test1.key2'), expected_cfg['test1']['key2'])
        self.assertAlmostEqual(cfg.getValue('test1.key3'), expected_cfg['test1']['key3'], places=6)
        self.assertEqual(cfg.getValue('test2.my_key'), expected_cfg['test2']['my_key'])
        self.assertEqual(cfg.getValue('test2.n_key'), expected_cfg['test2']['n_key'])
        self.assertAlmostEqual(cfg.getValue('test2.var_ref'), expected_cfg['test2']['var_ref'], places=6)
        self.assertEqual(cfg.getValue('solo_key'), expected_cfg['solo_key'])
        self.assertEqual(cfg.getValue('int_list'), expected_cfg['int_list'])
        self.assertEqual(cfg.getValue('uint_list'), expected_cfg['uint_list'])
        self.assertEqual(cfg.getValue('float_list'), expected_cfg['float_list'])
        self.assertEqual(cfg.getValue('uint64'), expected_cfg['uint64'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
