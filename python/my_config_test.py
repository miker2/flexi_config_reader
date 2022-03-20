import logging
import pprint
import unittest

import my_config_reader as mcr


class TestMyConfig(unittest.TestCase):
    def setUp(self):
        pass

    def test_1(self):
        my_config_example = '''
        struct test1
          key1 = "value"
          key2 = 1.342    # test comment here
          key3 = 10
          f = "none"
        end test1

        struct test2
          my_key = "foo"
          n_key = 1
          var_ref = $(test1.key3)
        end test2
        '''

        expected_cfg = {'test1': {'f': 'none',
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10},
                        'test2': {'my_key': 'foo',
                                  'n_key': 1,
                                  'var_ref': 10}}
        cfg = mcr.ConfigReader(my_config_example, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)

    def test_2(self):
        my_config_example = '''
        struct test1
          key1 = "value"
          key2 = 1.342    # test comment here
          key3 = 10
          f = "none"
          var_ref = $(test2.inner.val)
        end test1

        struct test2
          my_key = "foo"
          n_key = 1

          struct inner
            key = 1
            pair = "two"
            val = 1.232e10
          end inner
        end test2
        '''

        expected_cfg = {'test1': {'f': 'none',
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10,
                                  'var_ref': 1.232e10},
                        'test2': {'inner': {'key': 1,
                                            'pair': 'two',
                                            'val': 1.232e10},
                                  'my_key': 'foo',
                                  'n_key': 1}}
        cfg = mcr.ConfigReader(my_config_example, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)

    def test_3(self):
        my_config_example = '''
        struct test1
          key1 = "value"
          key2 = 1.342    # test comment here
          key3 = 10
          f = "none"
        end test1

        struct outer
          my_key = "foo"
          var_ref = $(outer.inner.test1.key)
          n_key = 1

          struct inner   # Another comment "here"
            key = 1
            #pair = "two"
            val = 1.232e10

            struct test1
              key = -5
            end test1
          end inner
        end outer

        struct test1
          different = 1
          other = 2
        end test1
        '''

        expected_cfg = {'test1': {'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10,
                                  'f': 'none',
                                  'different': 1,
                                  'other': 2},
                        'outer': {'my_key': 'foo',
                                  'var_ref': -5,
                                  'n_key': 1,

                                  'inner': {'key': 1,
                                            'val': 12320000000.0,
                                            'test1': {'key': -5}}
                                  }
                        }
        cfg = mcr.ConfigReader(my_config_example, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)


    def test_4(self):
        my_config_example = '''
        struct test1
          key1 = "value"
          key2 = 1.342    # test comment here
          key3 = 10
          f = "none"
        end test1

        struct protos
          proto joint_proto
            key1 = $VAR1
            key2   = $VAR2
            not_a_var = 27
            ref_var =    $(ref_in_struct.hx.add_another)  # This one is tricky!
          end joint_proto
        end protos

        struct outer
          my_key = "foo"
          n_key = 1

          struct inner   # Another comment "here"
            key = 1
            #pair = "two"
            val = 1.232e10

            struct test1
              key = -5
            end test1
          end inner
        end outer

        struct ref_in_struct
          key1 = -0.3492

          reference protos.joint_proto as hx
            $VAR1 = "0xdeadbeef"
            $VAR2 = "3"
            +new_var = 5
            +add_another = -3.14159
          end hx
        end ref_in_struct

        struct test1
          different = 1
          other = 2
        end test1

        '''

        expected_cfg = {'test1': {'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10,
                                  'f': 'none',
                                  'different': 1,
                                  'other': 2},
                        'outer': {'my_key': 'foo',
                                  'n_key': 1,
                                  'inner': {'key': 1,
                                            'val': 12320000000.0,
                                            'test1': {'key': -5}}
                                  },
                        'ref_in_struct': {'key1': -0.3492,
                                          'hx': {'key1': "0xdeadbeef",
                                                 'key2': "3",
                                                 'not_a_var': 27,
                                                 'ref_var': -3.14159,
                                                 'new_var': 5,
                                                 'add_another': -3.14159}
                                          },
                        'protos': {}
                        }


        cfg = mcr.ConfigReader(my_config_example, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)


    def test_5(self):
        my_config_example = '''
        struct protos

          proto joint_proto
            leg = $LEG
            dof = $DOF  # This should be $LEG.$DOF, but not supported yet
          end joint_proto

          proto leg_proto
            key_1 = "foo"
            key_2 = "bar"
            key_3 = $LEG

            reference protos.joint_proto as hx  # Can we make the 'hx' here be represented by $DOF?
              $DOF = "hx"
              +extra_key = 1e-3
            end hx
          end leg_proto
        end protos

        struct outer
          my_key = "foo"
          n_key = 1
          var_ref = $(outer.fl.key_3)

          struct inner   # Another comment "here"
            key = 1
            #pair = "two"
            val = 1.232e10

            struct test1
              key = -5
            end test1

            key_after = 3
          end inner

          key_between = 4

          reference protos.leg_proto as fl
            $LEG = "fl"
            $DOF = "hx"
          end fl
        end outer



        '''

        expected_cfg = {'outer': {'my_key': 'foo',
                                  'n_key': 1,
                                  'var_ref': "fl",
                                  'inner': {'key': 1,
                                            'val': 1.232e10,
                                            'test1': {'key': -5},
                                            'key_after': 3,
                                            },
                                  'key_between': 4,
                                  'fl': {'key_1': "foo",
                                         'key_2': "bar",
                                         'key_3': "fl",
                                         'hx': {'leg': "fl",
                                                'dof': "hx",
                                                'extra_key': 1e-3}
                                         }
                                  },
                        'protos': {}
                        }


        cfg = mcr.ConfigReader(my_config_example, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)


    def test_parse_from_file(self):
        input_file = "../examples/example.cfg"
        expected_cfg = {
            'protos' : {},
            'outer': {'my_key': "foo",
                      'n_key': 0x00FffF,
                      'inner': {'key': 0X000f,
                                'val': 1.232e10,
                                'test1':{'var_ref': 1e-3,
                                         'key': -5},
                                'key_after': [1, 2.345, 3, 4e-7, 5, "a string"]},
                      'key_between': 4,
                      'fl': {'key_1': "foo",
                             'key_2': "bar",
                             'key_3': "fl",
                             'hx': {'key1': "$CONFUSED",
                                    'leg': "fl",
                                    'dof': "fl.hx",
                                    'extra_key': 1e-3}
                             }
                      }
        }
        cfg = mcr.ConfigReader.parse_from_file(input_file, verbose=False)
        self.assertEqual(cfg.cfg, expected_cfg)


if __name__ == '__main__':
    unittest.main(verbosity=2)
