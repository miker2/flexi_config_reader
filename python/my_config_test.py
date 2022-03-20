

import logging
import pprint
import unittest

import my_config_reader as mcr


class TestMyConfig(unittest.TestCase):
    def setUp(self):
        pass

    def test_basic(self):

        print("----------------- Test 1 ------------------------------------------------------------------")
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

        cfg = mcr.ConfigReader(my_config_example, verbose=False)

        expected_cfg = {'test1': {'f': 'none',
                                  'key1': 'value',
                                  'key2': 1.342,
                                  'key3': 10},
                        'test2': {'my_key': 'foo',
                                  'n_key': 1,
                                  'var_ref': 10}}
        self.assertEqual(cfg.cfg, expected_cfg)

#unittest.main(verbosity=2)

#assert(False)
######### TESTS ########################################################################

print("----------------- Test 2 ------------------------------------------------------------------")
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

cfg = mcr.ConfigReader(my_config_example)
pprint.pprint(cfg.cfg)

print("----------------- Test 3 ------------------------------------------------------------------")
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

cfg = mcr.ConfigReader(my_config_example, verbose=False)
pprint.pprint(cfg.cfg)

print("----------------- Test 4 ------------------------------------------------------------------")
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
    key2 = $VAR2
    not_a_var = 27
    ref_var = $(ref_in_struct.hx.add_another)  # This one is tricky!
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

cfg = mcr.ConfigReader(my_config_example, verbose=False)
pprint.pprint(cfg.cfg)

print("----------------- Test 5 ------------------------------------------------------------------")
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

      key_afer = 3
    end inner

    key_between = 4

    reference protos.leg_proto as fl
      $LEG = "fl"
      $DOF = "hx"
    end fl
end outer



'''

cfg = mcr.ConfigReader(my_config_example, verbose=False)
pprint.pprint(cfg.cfg)

print("-------- File test ----------")
cfg = mcr.ConfigReader.parse_from_file("../examples/example.cfg", verbose=False)
pprint.pprint(cfg.cfg)
