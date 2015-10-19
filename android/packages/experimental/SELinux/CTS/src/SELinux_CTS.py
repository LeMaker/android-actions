import pdb
import re
from xml.etree.ElementTree import Element, SubElement, tostring

#define equivalents
TYPE = 0
ATTRIBUTE = 1
TYPEATTRIBUTE = 2
CLASS = 3
COMMON = 4
ALLOW_RULE = 5
NEVERALLOW_RULE = 6
OTHER = 7

#define helper methods
# advance_past_whitespace(): helper function to skip whitespace at current
# position in file.
# returns: the non-whitespace character at the file's new position
#TODO: should I deal with comments here as well?
def advance_past_whitespace(file_obj):
    c = file_obj.read(1)
    while c.isspace():
        c = file_obj.read(1)
    file_obj.seek(-1, 1)
    return c

# advance_until_whitespace(): helper function to grab the string represented
# by the current position in file until next whitespace.
# returns: string until next whitespace.  overlooks comments.
def advance_until_whitespace(file_obj):
    ret_string = ""
    c = file_obj.read(1)
    #TODO: make a better way to deal with ':' and ';'
    while not (c.isspace() or c == ':' or c == '' or c == ';'):
        #don't count comments
        if c == '#':
            file_obj.readline()
            return ret_string
        else:
            ret_string+=c
            c = file_obj.read(1)
    if not c == ':':
        file_obj.seek(-1, 1)
    return ret_string

# expand_avc_rule - takes a processed avc rule and converts it into a list of
# 4-tuples for use in an access check of form:
    # (source_type, target_type, class, permission)
def expand_avc_rule(policy, avc_rule):
    ret_list = [ ]

    #expand source_types
    source_types = avc_rule['source_types']['set']
    source_types = policy.expand_types(source_types)
    if(avc_rule['source_types']['flags']['complement']):
        #TODO: deal with negated 'self', not present in current policy.conf, though (I think)
        source_types = policy.types - source_types #complement these types
    if len(source_types) == 0:
        print "ERROR: source_types empty after expansion"
        print "Before: "
        print avc_rule['source_types']['set']
        return

    #expand target_types
    target_types = avc_rule['target_types']['set']
    target_types = policy.expand_types(target_types)
    if(avc_rule['target_types']['flags']['complement']):
        #TODO: deal with negated 'self', not present in current policy.conf, though (I think)
        target_types = policy.types - target_types #complement these types
    if len(target_types) == 0:
        print "ERROR: target_types empty after expansion"
        print "Before: "
        print avc_rule['target_types']['set']
        return

    # get classes
    rule_classes = avc_rule['classes']['set']
    if '' in rule_classes:
        print "FOUND EMPTY STRING IN CLASSES"
        print "Total sets:"
        print avc_rule['source_types']['set']
        print avc_rule['target_types']['set']
        print rule_classes
        print avc_rule['permissions']['set']

    if len(rule_classes) == 0:
        print "ERROR: empy set of object classes in avc rule"
        return

    # get permissions
    permissions = avc_rule['permissions']['set']
    if len(permissions) == 0:
        print "ERROR: empy set of permissions in avc rule\n"
        return

    #create the list with collosal nesting, n^4 baby!
    for s in source_types:
        for t in target_types:
            for c in rule_classes:
                if c == '':
                   continue
                #expand permissions on a per-class basis
                exp_permissions = policy.expand_permissions(c, permissions)
                if(avc_rule['permissions']['flags']['complement']):
                    exp_permissions = policy.classes[c] - exp_permissions
                if len(exp_permissions) == 0:
                    print "ERROR: permissions empty after expansion\n"
                    print "Before: "
                    print avc_rule['permissions']['set']
                    return
                for p in exp_permissions:
                    source = s
                    if t == 'self':
                        target = s
                    else:
                        target = t
                    obj_class = c
                    permission = p
                    ret_list.append((source, target, obj_class, permission))
    return ret_list

# expand_avc_rule - takes a processed avc rule and converts it into an xml
# representation with the information needed in a checkSELinuxAccess() call.
# (source_type, target_type, class, permission)
def expand_avc_rule_to_xml(policy, avc_rule, rule_name, rule_type):
    rule_xml = Element('avc_rule')
    rule_xml.set('name', rule_name)
    rule_xml.set('type', rule_type)

    #expand source_types
    source_types = avc_rule['source_types']['set']
    source_types = policy.expand_types(source_types)
    if(avc_rule['source_types']['flags']['complement']):
        #TODO: deal with negated 'self', not present in current policy.conf, though (I think)
        source_types = policy.types - source_types #complement these types
    if len(source_types) == 0:
        print "ERROR: source_types empty after expansion"
        print "Before: "
        print avc_rule['source_types']['set']
        return
    for s in source_types:
        elem = SubElement(rule_xml, 'type')
        elem.set('type', 'source')
        elem.text = s

    #expand target_types
    target_types = avc_rule['target_types']['set']
    target_types = policy.expand_types(target_types)
    if(avc_rule['target_types']['flags']['complement']):
        #TODO: deal with negated 'self', not present in current policy.conf, though (I think)
        target_types = policy.types - target_types #complement these types
    if len(target_types) == 0:
        print "ERROR: target_types empty after expansion"
        print "Before: "
        print avc_rule['target_types']['set']
        return
    for t in target_types:
        elem = SubElement(rule_xml, 'type')
        elem.set('type', 'target')
        elem.text = t

    # get classes
    rule_classes = avc_rule['classes']['set']

    if len(rule_classes) == 0:
        print "ERROR: empy set of object classes in avc rule"
        return

    # get permissions
    permissions = avc_rule['permissions']['set']
    if len(permissions) == 0:
        print "ERROR: empy set of permissions in avc rule\n"
        return

    # permissions are class-dependent, so bundled together
    for c in rule_classes:
        if c == '':
            print "AH!!! empty class found!\n"
            continue
        c_elem = SubElement(rule_xml, 'obj_class')
        c_elem.set('name', c)
        #expand permissions on a per-class basis
        exp_permissions = policy.expand_permissions(c, permissions)
        if(avc_rule['permissions']['flags']['complement']):
            exp_permissions = policy.classes[c] - exp_permissions
        if len(exp_permissions) == 0:
            print "ERROR: permissions empty after expansion\n"
            print "Before: "
            print avc_rule['permissions']['set']
            return

        for p in exp_permissions:
            p_elem = SubElement(c_elem, 'permission')
            p_elem.text = p

    return rule_xml

# expand_brackets - helper function which reads a file into a string until '{ }'s
# are balanced.  Brackets are removed from the string.  This function is based
# on the understanding that nested brackets in our policy.conf file occur only due
# to macro expansion, and we just need to know how much is included in a given
# policy sub-component.
def expand_brackets(file_obj):
    ret_string = ""
    c = file_obj.read(1)
    if not c == '{':
        print "Invalid bracket expression: " + c + "\n"
        file_obj.seek(-1, 1)
        return ""
    else:
        bracket_count = 1
    while bracket_count > 0:
        c = file_obj.read(1)
        if c == '{':
            bracket_count+=1
        elif c == '}':
            bracket_count-=1
        elif c == '#':
            #get rid of comment and replace with whitespace
            file_obj.readline()
            ret_string+=' '
        else:
            ret_string+=c
    return ret_string

# get_avc_rule_component - grabs the next component from an avc rule.  Basically,
# just reads the next word or bracketed set of words.
# returns - a set of the word, or words with metadata
def get_avc_rule_component(file_obj):
    ret_dict = { 'flags': {}, 'set': set() }
    c = advance_past_whitespace(file_obj)
    if c == '~':
        ret_dict['flags']['complement'] = True
        file_obj.read(1) #move to next char
        c = advance_past_whitespace(file_obj)
    else:
        ret_dict['flags']['complement'] = False
    if not c == '{':
        #TODO: change operations on file to operations on string?
        single_type =  advance_until_whitespace(file_obj)
        ret_dict['set'].add(single_type)
    else:
        mult_types = expand_brackets(file_obj)
        mult_types = mult_types.split()
        for t in mult_types:
            ret_dict['set'].add(t)
    return ret_dict

def get_line_type(line):
    if re.search(r'^type\s', line):
        return TYPE
    if re.search(r'^attribute\s', line):
        return ATTRIBUTE
    if re.search(r'^typeattribute\s', line):
        return TYPEATTRIBUTE
    if re.search(r'^class\s', line):
        return CLASS
    if re.search(r'^common\s', line):
        return COMMON
    if re.search(r'^allow\s', line):
        return ALLOW_RULE
    if re.search(r'^neverallow\s', line):
        return NEVERALLOW_RULE
    else:
        return OTHER

def is_multi_line(line_type):
    if line_type == CLASS:
        return True
    elif line_type == COMMON:
        return True
    elif line_type == ALLOW_RULE:
        return True
    elif line_type == NEVERALLOW_RULE:
        return True
    else:
        return False


#should only be called with file pointing to the 'i' in 'inherits' segment
def process_inherits_segment(file_obj):
    inherit_keyword = file_obj.read(8)
    if not inherit_keyword == 'inherits':
        #TODO: handle error, invalid class statement
        print "ERROR: invalid inherits statement"
        return
    else:
        advance_past_whitespace(file_obj)
        ret_inherited_common = advance_until_whitespace(file_obj)
        return ret_inherited_common

class SELinuxPolicy:

    def __init__(self):
        self.types = set()
        self.attributes = { }
        self.classes = { }
        self.common_classes = { }
        self.allow_rules = [ ]
        self.neverallow_rules = [ ]

    # create policy directly from policy file
    #@classmethod
    def from_file_name(self, policy_file_name):
        self.types = set()
        self.attributes = { }
        self.classes = { }
        self.common_classes = { }
        self.allow_rules = [ ]
        self.neverallow_rules = [ ]
        with open(policy_file_name, 'r') as policy_file:
            line = policy_file.readline()
            while line:
                line_type = get_line_type(line)
                if is_multi_line(line_type):
                    self.parse_multi_line(line, line_type, policy_file)
                else:
                    self.parse_single_line(line, line_type)
                line = policy_file.readline()

    # expand_permissions - generates the actual permission set based on the listed
    # permissions with wildcards and the given class on which they're based.
    def expand_permissions(self, obj_class, permission_set):
        ret_set = set()
        neg_set = set()
        for p in permission_set:
            if p[0] == '-':
                real_p = p[1:]
                if real_p in self.classes[obj_class]:
                    neg_set.add(real_p)
                else:
                    print "ERROR: invalid permission in avc rule " + real_t + "\n"
                    return
            else:
                if p in self.classes[obj_class]:
                    ret_set.add(p)
                elif p == '*':  #pretty sure this can't be negated? eg -*
                    ret_set |= self.classes[obj_class]  #All of the permissions
                else:
                    print "ERROR: invalid permission in avc rule " + p + "\n"
                    return
        return ret_set - neg_set

    # expand_types - generates the actual type set based on the listed types,
    # attributes, wildcards and negation.  self is left as-is, and is processed
    # specially when generating checkAccess() 4-tuples
    def expand_types(self, type_set):
        ret_set = set()
        neg_set = set()
        for t in type_set:
            if t[0] == '-':
                real_t = t[1:]
                if real_t in self.attributes:
                    neg_set |= self.attributes[real_t]
                elif real_t in self.types:
                    neg_set.add(real_t)
                elif real_t == 'self':
                    ret_set |= real_t
                else:
                    print "ERROR: invalid type in avc rule " + real_t + "\nTYPE SET:"
                    print type_set
                    return
            else:
                if t in self.attributes:
                     ret_set |= self.attributes[t]
                elif t in self.types:
                    ret_set.add(t)
                elif t == 'self':
                    ret_set.add(t)
                elif t == '*':  #pretty sure this can't be negated?
                     ret_set |= self.types  #All of the types
                else:
                    print "ERROR: invalid type in avc rule " + t + "\nTYPE SET"
                    print type_set
                    return
        return ret_set - neg_set

    def parse_multi_line(self, line, line_type, file_obj):
        if line_type == CLASS:
            self.process_class_line(line, file_obj)
        elif line_type == COMMON:
            self.process_common_line(line, file_obj)
        elif line_type == ALLOW_RULE:
            self.process_avc_rule_line(line, file_obj)
        elif line_type == NEVERALLOW_RULE:
            self.process_avc_rule_line(line, file_obj)
        else:
            print "Error: This is not a multi-line input"

    def parse_single_line(self, line, line_type):
        if line_type == TYPE:
            self.process_type_line(line)
        elif line_type == ATTRIBUTE:
            self.process_attribute_line(line)
        elif line_type == TYPEATTRIBUTE:
            self.process_typeattribute_line(line)
        return

    def process_attribute_line(self, line):
        match = re.search(r'^attribute\s+(.+);', line)
        if match:
            declared_attribute = match.group(1)
            self.attributes[declared_attribute] = set()
        else:
            #TODO: handle error? (no state changed)
            return

    def process_class_line(self, line, file_obj):
        match = re.search(r'^class\s([^\s]+)\s(.*$)', line)
        if match:
            declared_class = match.group(1)
            #first class declaration has no perms
            if not declared_class in self.classes:
                self.classes[declared_class] = set()
                return
            else:
                #need to parse file from after class name until end of '{ }'s
                file_obj.seek(-(len(match.group(2)) + 1), 1)
                c = advance_past_whitespace(file_obj)
                if not (c == 'i' or c == '{'):
                    print "ERROR: invalid class statement"
                    return
                elif c == 'i':
                    #add inherited permissions
                    inherited = process_inherits_segment(file_obj)
                    self.classes[declared_class] |= self.common_classes[inherited]
                    c = advance_past_whitespace(file_obj)
                if c == '{':
                    permissions = expand_brackets(file_obj)
                    permissions = re.sub(r'#[^\n]*\n','\n' , permissions) #get rid of all comments
                    permissions = permissions.split()
                    for p in permissions:
                        self.classes[declared_class].add(p)

    def process_common_line(self, line, file_obj):
        match = re.search(r'^common\s([^\s]+)(.*$)', line)
        if match:
            declared_common_class = match.group(1)
            #TODO: common classes should only be declared once...
            if not declared_common_class in self.common_classes:
                self.common_classes[declared_common_class] = set()
            #need to parse file from after common_class name until end of '{ }'s
            file_obj.seek(-(len(match.group(2)) + 1), 1)
            c = advance_past_whitespace(file_obj)
            if not c == '{':
                print "ERROR: invalid common statement"
                return
            permissions = expand_brackets(file_obj)
            permissions = permissions.split()
            for p in permissions:
                self.common_classes[declared_common_class].add(p)
        return

    def process_avc_rule_line(self, line, file_obj):
        match = re.search(r'^(never)?allow\s(.*$)', line)
        if match:
            if(match.group(1)):
                rule_type = 'neverallow'
            else:
                rule_type = 'allow'
            #need to parse file from after class name until end of '{ }'s
            file_obj.seek(-(len(match.group(2)) + 1), 1)

            #grab source type(s)
            source_types = get_avc_rule_component(file_obj)
            if len(source_types['set']) == 0:
                print "ERROR: no source types for avc rule at line: " + line
                return

            #grab target type(s)
            target_types = get_avc_rule_component(file_obj)
            if len(target_types['set']) == 0:
                print "ERROR: no target types for avc rule at line: " + line
                return

            #skip ':' potentially already handled by advance_until_whitespace
            c = advance_past_whitespace(file_obj)
            if c == ':':
                file_obj.read(1)

            #grab class(es)
            classes = get_avc_rule_component(file_obj)
            if len(classes['set']) == 0:
                print "ERROR: no classes for avc rule at line: " + line
                return

            #grab permission(s)
            permissions = get_avc_rule_component(file_obj)
            if len(permissions['set']) == 0:
                print "ERROR: no permissions for avc rule at line: " + line
                return
            rule_dict = {
                'source_types': source_types,
                'target_types': target_types,
                'classes': classes,
                'permissions': permissions }

            if rule_type == 'allow':
                self.allow_rules.append(rule_dict)
            elif rule_type == 'neverallow':
                self.neverallow_rules.append(rule_dict)

    def process_type_line(self, line):
        #TODO: add support for aliases (not yet in current policy.conf)
        match = re.search(r'^type\s([^,]+),?(.*);', line)
        if match:
            declared_type = match.group(1)
            self.types.add(declared_type)
            if match.group(2):
                declared_attributes = match.group(2)
                declared_attributes = declared_attributes.replace(" ", "") #remove whitespace
                declared_attributes = declared_attributes.split(',') #separate based on delimiter
                for a in declared_attributes:
                    if not a in self.attributes:
                        #TODO: hanlde error? attribute should already exist
                        self.attributes[a] = set()
                    self.attributes[a].add(declared_type)
        else:
            #TODO: handle error? (no state changed)
            return

    def process_typeattribute_line(self, line):
        match = re.search(r'^typeattribute\s([^\s]+)\s(.*);', line)
        if match:
            declared_type = match.group(1)
            if not declared_type in self.types:
                #TODO: handle error? type should already exist
                self.types.add(declared_type)
            if match.group(2):
                declared_attributes = match.group(2)
                declared_attributes = declared_attributes.replace(" ", "") #remove whitespace
                declared_attributes = declared_attributes.split(',') #separate based on delimiter
                for a in declared_attributes:
                    if not a in self.attributes:
                        #TODO: hanlde error? attribute should already exist
                        self.attributes[a] = set()
                    self.attributes[a].add(declared_type)
            else:
                return
        else:
            #TODO: handle error? (no state changed)
            return
