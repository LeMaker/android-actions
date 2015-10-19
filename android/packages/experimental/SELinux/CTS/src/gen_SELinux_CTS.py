#!/usr/bin/python
# genCheckAccessCTS.py - takes an input SELinux policy.conf file and generates
# an XML file based on the allow and neverallow rules.  The file contains rules,
# which are created by expanding the SELinux rule notation into the individual
# components which a checkAccess() check, that a policy manager would have to
# perform, needs.
#
# This test does not work with all valid SELinux policy.conf files.  It is meant
# to simply use a given AOSP generated policy.conf file to create sets
# representing the policy's types, attributes, classes and permissions, which
# are used to expand the allow and neverallow rules found.  For a full parser
# and compiler of SELinux, see external/checkpolicy.
# @dcashman

import pdb
import re
import sys
from xml.etree.ElementTree import Element, SubElement, tostring
from xml.dom import minidom

import SELinux_CTS
from SELinux_CTS import SELinuxPolicy

usage = "Usage: ./gen_SELinux_CTS.py input_policy_file output_xml_avc_rules_file"

if __name__ == "__main__":
    # check usage
    if len(sys.argv) != 3:
        print usage
        exit()
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    policy = SELinuxPolicy()
    policy.from_file_name(input_file) #load data from file

    # expand rules into 4-tuples for SELinux.h checkAccess() check
    xml_root = Element('SELinux_AVC_Rules')
    count = 1
    for a in policy.allow_rules:
        expanded_xml = SELinux_CTS.expand_avc_rule_to_xml(policy, a, str(count), 'allow')
        if len(expanded_xml):
            xml_root.append(expanded_xml)
            count += 1
    count = 1
    for n in policy.neverallow_rules:
        expanded_xml = SELinux_CTS.expand_avc_rule_to_xml(policy, n, str(count), 'neverallow')
        if len(expanded_xml):
            xml_root.append(expanded_xml)
            count += 1

    #print out the xml file
    s = tostring(xml_root)
    s_parsed = minidom.parseString(s)
    output = s_parsed.toprettyxml(indent="    ")
    with open(output_file, 'w') as out_file:
        out_file.write(output)
