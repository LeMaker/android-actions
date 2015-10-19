#!/usr/bin/python
import sys
sys.path.append('../src')
import unittest
import SELinux_CTS
from SELinux_CTS import SELinuxPolicy

policy_file_name = 'policy_test.conf'
types = set([
        'bluetooth',
        'healthd',
        'healthd_exec',
        'testTYPE' ])  #testTYPE added for neverallow rule to make sense
attributes = {
    'domain': set(['bluetooth', 'healthd', 'testTYPE']),
    'unconfineddomain': set(['bluetooth']),
    'appdomain': set(['bluetooth', 'testTYPE']),
    'file_type': set(['healthd_exec']),
    'exec_type': set(['healthd_exec']) }
common_classes = {
    'file': set([
            'ioctl',
            'read',
            'write',
            'create',
            'getattr',
            'setattr',
            'lock',
            'relabelfrom',
            'relabelto',
            'append',
            'unlink',
            'link',
            'rename',
            'execute',
            'swapon',
            'quotaon',
            'mounton' ]) }
classes = {
    'capability': set([
            'chown',
            'dac_override',
            'dac_read_search',
            'fowner',
            'fsetid',
            'kill',
            'setgid',
            'setuid',
            'setpcap',
            'linux_immutable',
            'net_bind_service',
            'net_broadcast',
            'net_admin',
            'net_raw',
            'ipc_lock',
            'ipc_owner',
            'sys_module',
            'sys_rawio',
            'sys_chroot',
            'sys_ptrace',
            'sys_pacct',
            'sys_admin',
            'sys_boot',
            'sys_nice',
            'sys_resource',
            'sys_time',
            'sys_tty_config',
            'mknod',
            'lease',
            'audit_write',
            'audit_control',
            'setfcap' ]),
    'file': (set([
                'execute_no_trans',
                'entrypoint',
                'execmod',
                'open',
                'audit_access' ]) | common_classes['file']) }

# allow healthd healthd_exec:file { entrypoint read execute };
allow_rules = [
    { 'source_types': {
        'set': set([
                'healthd']),
        'flags': { 'complement': False } },
      'target_types': {
        'set': set([
                'healthd_exec']),
        'flags': { 'complement': False } },
      'classes': {
        'set': set([
                'file']),
        'flags': { 'complement': False } },
      'permissions': {
        'set': set([
                'entrypoint',
                'read',
                'execute' ]),
        'flags': { 'complement': False } } } ]

# neverallow { appdomain -unconfineddomain -bluetooth } self:capability *;
neverallow_rules = [
    { 'source_types': {
        'set': set([
                'appdomain',
                '-unconfineddomain',
                '-bluetooth' ]),
        'flags': { 'complement': False } },
      'target_types': {
        'set': set([
                'self']),
        'flags': { 'complement': False } },
      'classes': {
        'set': set([
                'capability']),
        'flags': { 'complement': False } },
      'permissions': {
        'set': set([
                '*' ]),
        'flags': { 'complement': False } } } ]

expected_final_allow_list = [
        [ ('healthd', 'healthd_exec', 'file', 'entrypoint'),
                ('healthd', 'healthd_exec', 'file', 'read'),
                ('healthd', 'healthd_exec', 'file', 'execute') ] ]

expected_final_neverallow_list = [
        [ ('testTYPE', 'testTYPE', 'capability', 'chown'),
                ('testTYPE', 'testTYPE', 'capability', 'dac_override'),
                ('testTYPE', 'testTYPE', 'capability', 'dac_read_search'),
                ('testTYPE', 'testTYPE', 'capability', 'fowner'),
                ('testTYPE', 'testTYPE', 'capability', 'fsetid'),
                ('testTYPE', 'testTYPE', 'capability', 'kill'),
                ('testTYPE', 'testTYPE', 'capability', 'setgid'),
                ('testTYPE', 'testTYPE', 'capability', 'setuid'),
                ('testTYPE', 'testTYPE', 'capability', 'setpcap'),
                ('testTYPE', 'testTYPE', 'capability', 'linux_immutable'),
                ('testTYPE', 'testTYPE', 'capability', 'net_bind_service'),
                ('testTYPE', 'testTYPE', 'capability', 'net_broadcast'),
                ('testTYPE', 'testTYPE', 'capability', 'net_admin'),
                ('testTYPE', 'testTYPE', 'capability', 'net_raw'),
                ('testTYPE', 'testTYPE', 'capability', 'ipc_lock'),
                ('testTYPE', 'testTYPE', 'capability', 'ipc_owner'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_module'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_rawio'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_chroot'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_ptrace'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_pacct'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_admin'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_boot'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_nice'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_resource'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_time'),
                ('testTYPE', 'testTYPE', 'capability', 'sys_tty_config'),
                ('testTYPE', 'testTYPE', 'capability', 'mknod'),
                ('testTYPE', 'testTYPE', 'capability', 'lease'),
                ('testTYPE', 'testTYPE', 'capability', 'audit_write'),
                ('testTYPE', 'testTYPE', 'capability', 'audit_control'),
                ('testTYPE', 'testTYPE', 'capability', 'setfcap') ] ]


class SELinuxPolicyTests(unittest.TestCase):


    def setUp(self):
        self.test_policy = SELinuxPolicy()
        self.test_file = open(policy_file_name, 'r')
        self.test_policy.types = types
        self.test_policy.attributes = attributes
        self.test_policy.common_classes = common_classes
        self.test_policy.classes = classes
        self.test_policy.allow_rules = allow_rules
        self.test_policy.neverallow_rules = neverallow_rules
        return

    def testExpandAvcRule(self):
        #TODO: add more examples here to cover different cases
        expanded_allow_list = SELinux_CTS.expand_avc_rule(self.test_policy, self.test_policy.allow_rules[0])
        for a in expected_final_allow_list[0]:
            self.failUnless(a in expanded_allow_list)
        expanded_neverallow_list = SELinux_CTS.expand_avc_rule(self.test_policy, self.test_policy.neverallow_rules[0])
        for n in expected_final_neverallow_list[0]:
            self.failUnless(n in expanded_neverallow_list)

    def testExpandBrackets(self):
        #test position without bracket:
        self.test_file.seek(279)
        self.failIf(SELinux_CTS.expand_brackets(self.test_file))

        #test position with bracket:
        self.test_file.seek(26123)
        self.failUnless(SELinux_CTS.expand_brackets(self.test_file) == " entrypoint read execute ")

        #test position with nested brackets:
        self.test_file.seek(26873)
        self.failUnless(SELinux_CTS.expand_brackets(self.test_file)
               == " dir   chr_file blk_file   file lnk_file sock_file fifo_file   ")

    def testGetAvcRuleComponent(self):
        #test against normal ('allow healthd healthd_exec:file ...)
        self.test_file.seek(26096)
        normal_src = { 'flags': { 'complement': False },
                'set': set(['healthd']) }
        normal_tgt = { 'flags': { 'complement': False },
                'set': set(['healthd_exec']) }
        normal_class = { 'flags': { 'complement': False },
                'set': set(['file']) }
        normal_perm = { 'flags': { 'complement': False },
                'set': set(['entrypoint', 'read', 'execute']) }
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == normal_src)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == normal_tgt)
        c = SELinux_CTS.advance_past_whitespace(self.test_file)
        if c == ':':
            self.test_file.read(1)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == normal_class)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == normal_perm)

        #test against 'hard' ('init {fs_type  ...' )
        self.test_file.seek(26838)
        hard_src = { 'flags': { 'complement': False },
                'set': set(['init']) }
        hard_tgt = { 'flags': { 'complement': False },
                'set': set(['fs_type', 'dev_type', 'file_type']) }
        hard_class = { 'flags': { 'complement': False },
                'set': set(['dir', 'chr_file', 'blk_file', 'file', 'lnk_file', 'sock_file', 'fifo_file']) }
        hard_perm = { 'flags': { 'complement': False },
                'set': set(['relabelto']) }
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == hard_src)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == hard_tgt)
        #mimic ':' check:
        c = SELinux_CTS.advance_past_whitespace(self.test_file)
        if c == ':':
            self.test_file.read(1)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == hard_class)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == hard_perm)

        #test against 'multi-line' ('init {fs_type  ...' )
        self.test_file.seek(26967)
        multi_src = { 'flags': { 'complement': False },
                'set': set(['appdomain', '-unconfineddomain']) }
        multi_tgt = { 'flags': { 'complement': False },
                'set': set(['audio_device', 'camera_device', 'dm_device', 'radio_device', 'gps_device', 'rpmsg_device']) }
        multi_class = { 'flags': { 'complement': False },
                'set': set(['chr_file']) }
        multi_perm = { 'flags': { 'complement': False },
                'set': set(['read', 'write']) }
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == multi_src)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == multi_tgt)
        c = SELinux_CTS.advance_past_whitespace(self.test_file)
        if c == ':':
            self.test_file.read(1)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == multi_class)
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == multi_perm)

        #test against 'complement'
        self.test_file.seek(26806)
        complement = { 'flags': { 'complement': True },
                'set': set(['entrypoint', 'relabelto']) }
        self.failUnless(SELinux_CTS.get_avc_rule_component(self.test_file)
            == complement)

    def testGetLineType(self):
        self.failUnless(SELinux_CTS.get_line_type('type bluetooth, domain;')
                == SELinux_CTS.TYPE)
        self.failUnless(SELinux_CTS.get_line_type('attribute unconfineddomain;')
                == SELinux_CTS.ATTRIBUTE)
        self.failUnless(SELinux_CTS.get_line_type('typeattribute bluetooth appdomain;')
                == SELinux_CTS.TYPEATTRIBUTE)
        self.failUnless(SELinux_CTS.get_line_type('class file')
                == SELinux_CTS.CLASS)
        self.failUnless(SELinux_CTS.get_line_type('common file')
                == SELinux_CTS.COMMON)
        self.failUnless(SELinux_CTS.get_line_type('allow healthd healthd_exec:file { entrypoint read execute };')
                == SELinux_CTS.ALLOW_RULE)
        self.failUnless(SELinux_CTS.get_line_type('neverallow { appdomain -unconfineddomain -bluetooth } self:capability *;')
                == SELinux_CTS.NEVERALLOW_RULE)
        self.failUnless(SELinux_CTS.get_line_type('# FLASK')
                == SELinux_CTS.OTHER)

    def testIsMultiLine(self):
        self.failIf(SELinux_CTS.is_multi_line(SELinux_CTS.TYPE))
        self.failIf(SELinux_CTS.is_multi_line(SELinux_CTS.ATTRIBUTE))
        self.failIf(SELinux_CTS.is_multi_line(SELinux_CTS.TYPEATTRIBUTE))
        self.failUnless(SELinux_CTS.is_multi_line(SELinux_CTS.CLASS))
        self.failUnless(SELinux_CTS.is_multi_line(SELinux_CTS.COMMON))
        self.failUnless(SELinux_CTS.is_multi_line(SELinux_CTS.ALLOW_RULE))
        self.failUnless(SELinux_CTS.is_multi_line(SELinux_CTS.NEVERALLOW_RULE))
        self.failIf(SELinux_CTS.is_multi_line(SELinux_CTS.OTHER))

    def testProcessInheritsSegment(self):
        inherit_offset = 448 # needs changing if file changes
        self.test_file.seek(inherit_offset, 0)
        inherit_result = SELinux_CTS.process_inherits_segment(self.test_file)
        self.failUnless(inherit_result == 'file')
        return

    def testFromFileName(self):
        #using a special file, since the test_file has some lines which don't 'jive'
        clean_policy_file = 'policy_clean_test.conf'
        from_file_policy = SELinuxPolicy()
        from_file_policy.from_file_name(clean_policy_file)
        self.failUnless(from_file_policy.types == self.test_policy.types)
        self.failUnless(from_file_policy.attributes == self.test_policy.attributes)
        self.failUnless(from_file_policy.classes == self.test_policy.classes)
        self.failUnless(from_file_policy.common_classes == self.test_policy.common_classes)
        self.failUnless(from_file_policy.allow_rules == self.test_policy.allow_rules)
        self.failUnless(from_file_policy.neverallow_rules == self.test_policy.neverallow_rules)

    def testExpandPermissions(self):
        #test general case
        test_class_obj = 'file'
        general_set = set(['read', 'write', 'execute'])
        expanded_general_set = general_set
        self.failUnless(self.test_policy.expand_permissions(test_class_obj, general_set)
                == general_set)
        star_set = set(['*'])
        expanded_star_set = self.test_policy.classes['file'] #everything in the class
        self.failUnless(self.test_policy.expand_permissions(test_class_obj, star_set)
                == expanded_star_set)
        complement_set = set(['*', '-open'])
        expanded_complement_set = self.test_policy.classes['file'] - set(['open'])
        self.failUnless(self.test_policy.expand_permissions(test_class_obj, complement_set)
                == expanded_complement_set)

    def testExpandTypes(self):

        #test general case and '-' handling
        test_source_set = set([
                'domain',
                '-bluetooth' ])
        expanded_test_source_set = set([
                'healthd', 'testTYPE' ])
        self.failUnless(self.test_policy.expand_types(test_source_set) == expanded_test_source_set)

        #test '*' handling
        test_source_set = set([ '*' ])
        expanded_test_source_set = set([
                'bluetooth', 'healthd', 'testTYPE' ])
        self.failUnless(self.test_policy.expand_types(test_source_set) == types)
        #test - handling
        test_source_set = set([
                '*',
                '-bluetooth'])
        expanded_test_source_set = set([
                'healthd', 'healthd_exec', 'testTYPE' ])
        self.failUnless(self.test_policy.expand_types(test_source_set) == expanded_test_source_set)

    def testProcessAttributeLine(self):
        attribute_policy = SELinuxPolicy()
        #test with 'normal input'
        test_normal_string = 'attribute TEST_att;'
        test_attribute = 'TEST_att'
        attribute_policy.process_attribute_line(test_normal_string)
        self.failUnless( test_attribute in attribute_policy.attributes)
        #TODO: test on bogus inputs

    def testProcessClassLine(self):
        class_policy = SELinuxPolicy()
        #offsets need changing if test file changes
        common_offset  = 279
        class_initial_offset  = 212
        class_perm_offset = 437
        self.test_file.seek(common_offset, 0)
        line = self.test_file.readline()
        class_policy.process_common_line(line, self.test_file)
        self.test_file.seek(class_initial_offset, 0)
        line = self.test_file.readline()
        class_policy.process_class_line(line, self.test_file)
        self.failUnless('file' in class_policy.classes)
        self.test_file.seek(class_perm_offset, 0)
        line = self.test_file.readline()
        class_policy.process_class_line(line, self.test_file)
        self.failUnless(class_policy.classes['file'] == classes['file'])

    def testProcessCommonLine(self):
        common_policy = SELinuxPolicy()
        common_offset  = 279 # needs changing if file changes
        self.test_file.seek(common_offset, 0)
        line = self.test_file.readline()
        common_policy.process_common_line(line, self.test_file)
        self.failUnless('file' in common_policy.common_classes )
        self.failUnless(common_policy.common_classes['file'] == common_classes['file'])

    def testProcessAvcRuleLine(self):
        avc_policy = SELinuxPolicy()
        allow_offset  =  26091 # needs changing if file changes
        neverallow_offset  = 26311  # needs changing if file changes
        self.test_file.seek(allow_offset, 0)
        line = self.test_file.readline()
        avc_policy.process_avc_rule_line(line, self.test_file)
        self.failUnless(avc_policy.allow_rules[0] == allow_rules[0] ) # always '0'?
        self.test_file.seek(neverallow_offset, 0)
        line = self.test_file.readline()
        avc_policy.process_avc_rule_line(line, self.test_file)
        self.failUnless(avc_policy.neverallow_rules[0] == neverallow_rules[0] ) # always '0'?

    def testProcessTypeLine(self):
        type_policy = SELinuxPolicy()
        test_normal_string = 'type TEST_type, TEST_att1, TEST_att2;'
        test_type = 'TEST_type'
        test_atts = ['TEST_att1', 'TEST_att2']
        #test with 'normal input'
        type_policy.process_type_line(test_normal_string)
        self.failUnless(test_type in type_policy.types)
        for a in test_atts:
            self.failUnless(a in type_policy.attributes)
            self.failUnless(test_type in type_policy.attributes[a])
        #TODO: test with domain only, no attributes
        # and test on bogus inputs

    def testProcessTypeattributeLine(self):
        typ_att_policy = SELinuxPolicy()
        test_normal_string = 'typeattribute TEST_type TEST_att1, TEST_att2;'
        test_type = 'TEST_type'
        test_atts = ['TEST_att1', 'TEST_att2']
        #test with 'normal input' (type should already be declared)
        typ_att_policy.process_type_line('type ' + test_type + ';')
        typ_att_policy.process_typeattribute_line(test_normal_string)
        self.failUnless(test_type in typ_att_policy.types)
        for a in test_atts:
            self.failUnless(a in typ_att_policy.attributes)
            self.failUnless(test_type in typ_att_policy.attributes[a])
        #TODO: test with domain only, no attributes
        # and test on bogus inputs

def main():
    unittest.main()

if __name__ == '__main__':
    main()
