#!/usr/bin/env python3
import re
import sys
import os

class NoH19Error(RuntimeError):
    def __init__(self, msg):
        super().__init__(msg)

def vhdl_editor(blob):
    # remove JTAGH19 or JTAGH19SOFT instantiation; unfortunately we cannot simply
    # replace it by JTAGH19EMUL because synthesis of the rewritten top somehow
    # has no knowledge of other design files (which are not radiant-internal libraries)
    # so it doesn't know where to find JTAGH19EMUL.
    # Thus, the top-level design must instantiate it and this editor
    #  - removes the JTAGH19[SOFT] instance
    #  - rewires JTAGH19EMUL's ports to the signals
    #    which the JTAGH19 used. E.g., jtckemu -> jtck (etc.)
    # This pattern can be used to remove the VHDL JTAGH19 or JTAGH19SOFT instantiation
    jtagh19_inst=re.compile(r'\b[a-z0-9_]+\s*:\s*(component)?\s*\bjtagh19(soft)?\b([^;]|\s)*[;]',re.IGNORECASE)
    if ( jtagh19_inst.search(blob) is None ):
        raise NoH19Error("vhdl_editor: no jtagh19 or jtagh19soft found")

    # This pattern can be used to change the port map of JTAGH19EMUL. Currently we assume we know
    # the port names (simply jtck, jtdi, jshift, ...) but we could extract from the instantiation
    # above before removing it.
    portmap=re.compile(r'[=][>]\s*(jtck|jrstn|jtdi|jshift|jupdate|jce2|er2_tdo|ip_enable)emu\s*([,)])',re.IGNORECASE)
    if ( portmap.search(blob) is None ):
        raise NoH19Error("vhdl_editor: no emulation port map found")
    blob=jtagh19_inst.sub(r'',blob)
    blob=portmap.sub(r'=>\1\2', blob)
    return blob

def verilog_editor(blob):
    onam=[]
    plst=['JTCK','JTDI','JRSTN','JSHIFT','JUPDATE','JCE2','ER2_TDO','IP_ENABLE']
    # <space> ( <wire_name> )
    pmap_pat =r'\s*([(][^)]*[)])'
    #  .<portname> ( <wire_name> )
    v_portmap=r'[.](' + '|'.join(plst) + r')' + pmap_pat
    # construct a dict <port-name> -> <original_port_map>
    pmap=dict()
    # no '.' between module name and ';' - omit declarations!
    mod_body_patt=r'([^;]*[.])+[^;]*[;]'
    orig_patt = re.compile(r'\bJTAGH19(SOFT)?\b' + mod_body_patt)
    # reveal may rename the module by appending stuff!
    # Allow non-space continuation after JTAGH19EMUL
    emul_patt = re.compile(r'JTAGH19EMUL' + mod_body_patt)
    orig_inst = orig_patt.search(blob)
    if ( orig_inst is None ):
        raise NoH19Error("verilog_editor: no jtagh19 or jtagh19soft found")
    # no '.' between module name and ';' - omit declarations!
    emul_inst = emul_patt.search(blob)
    if ( emul_inst is None ):
        raise NoH19Error("verilog_editor: no jtagh19emul found")
    # for each port find its mapping in the JTAGH19 or JTAGH19SOFT instantiation
    for port in plst:
        m=re.search(r'[.]'+ port + pmap_pat, orig_inst.group(0))
        pmap[port]=m.group(1)
    # replace the port mappings in the emulation instantiation
    # by the original ones
    def s(m):
        return '.' + m.group(1) + pmap[m.group(1)]
    print("emulinst ", emul_inst)
    emulstr_new=re.sub(v_portmap, s, emul_inst.group(0))
    print("emulstr_new ", emulstr_new)
    # replace the emul instantiation by the remapped one
    # and remove the original instantiation
    def s(m):
        if (m.group(1)=='JTAGH19EMUL'):
            return emulstr_new
        else:
            return ''
    # Again: allow non-space continuation after EMUL - reveal may rename!
    blob = re.sub(r'(JTAGH19(SOFT|EMUL))'+mod_body_patt,s,blob)
    # Remove top-level ports for TCK(SOFT)? & friends
    # a port declaration PD (e.g., 'input|output wire TCKSOFT')
    # may either be the first declaration in which case it is preceded
    # by a '(' and in which case we eat a trailing ',' if it is present
    # or the declaration must be preceded by a optional comma (which we
    # eat and may be followed byt ')', i.e.,
    #
    # Either  '(' PD [',']
    # OR      [','] PD [')']
    #
    # We eat the declaration as well as trailing or leading commas but
    # leave the parenthesis.
    def s1(m):
        cls_paren = m.group(9)
        opn_paren = m.group(1)
        if ( not opn_paren is None ):
            return "("
        return cls_paren
    prtdecl = r'\s*\b(in|out)put\b\s+\bwire\b\s+(TCK|TMS|TDI|TDO)(SOFT)?\s*'
    blob = re.sub(r'([(]' + prtdecl + r'[,]?)|([,]?' + prtdecl + r'([)]?))', s1, blob)
    return blob

def edit(f, editor):
    blob = f.read()
    blob = editor(blob)
    f.truncate(0)
    f.seek(0,os.SEEK_SET)
    f.seek(0)
    f.write(blob)

if __name__ == "__main__":
    if ( len(sys.argv) < 2 ):
        raise RuntimeError("Need filename argument")
    fnam = sys.argv[1]
    if ( not re.search('[.]v$', fnam, re.IGNORECASE) is None ):
        print("Editing VERILOG")
        editor=verilog_editor
    elif ( not re.search('[.]vhdl?$', fnam, re.IGNORECASE) is None ):
        print("Editing VHDL")
        editor=vhdl_editor
    else:
        raise RuntimeError("Unrecognized file name suffix")
    with open(sys.argv[1],'r+') as f:
        try:
            edit(f, editor)
        except NoH19Error as e:
            # not really an error; they simply have debugging
            # disabled
            print(e.args[0])
