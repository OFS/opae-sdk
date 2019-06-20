##########################
#
# Common Util API
#
##########################
import os
import sys
import platform
import terminal
import array
import socket
import random
import inspect
from sys import platform as _platform
from subprocess import Popen
from ctypes import *
    
def print_new_line() :

    terminal.printing("", terminal.MSG_TYPE.NULL, terminal.BCOLORS.INFO, 0, None, False)

def print_info(string, space=0, file=None, alternate_color=False) :

    terminal.printing(string, terminal.MSG_TYPE.INFO, terminal.BCOLORS.INFO, space, file, alternate_color)
    
def print_warning(string, space=0, file=None) :

    terminal.printing(string, terminal.MSG_TYPE.WARNING, terminal.BCOLORS.WARNING, space, file)
    
def print_error(string, space=0, file=None) :

    terminal.printing(string, terminal.MSG_TYPE.ERROR, terminal.BCOLORS.ERROR, space, file)

def print_prompt(string, space=0, file=None) :

    terminal.printing(string, terminal.MSG_TYPE.NULL, terminal.BCOLORS.PROMPT, space, file)
        
def exception_handler(etype, value, tb) :

    pass
    
def assert_in_error(boolean, string, *arg) :

    if boolean :
    
        pass
        
    elif len(string) :
    
        if len(arg) :
            string = string % (arg)
    
        # contruct message 
        caller = inspect.stack()[1]
        module = os.path.basename(caller[1])
        info = "Module: %s, Function: %s, Line: %d" % (module, caller[3], caller[2])
        msg = "%s\n       %s" % (info, string)
        print_error(msg, space=0)
        sys.excepthook = exception_handler
        assert False, msg
        
    else :
    
        # contruct message 
        caller = inspect.stack()[1]
        module = os.path.basename(caller[1])
        msg = "Module: %s, Function: %s, Line: %d" % (module, caller[3], caller[2])
        print_error(msg, space=0)
        sys.excepthook = exception_handler
        assert False, msg
        
def get_filename(fullpath, space=0) :

    fullpath = fullpath.replace("\\", "/")
    filename = fullpath
    if len (fullpath) :
        index = fullpath.rfind("/")
        if index != 0 :
            filename = fullpath[index+1:]
    else :
        print_error("Input is NULL function get_filename()", space = space) 
        sys.exit(-1)
    return filename

def is_windows_os() :

    return platform.system() == "Windows"
    
def check_extension(file, extension) :

    status = True
    file_char_count = len(file)
    extension_char_count = len(extension)
    if file_char_count == 0 or \
        extension_char_count == 0 or \
        file_char_count < (extension_char_count + 1) or \
        extension != file[(file_char_count-extension_char_count):] :
        status = False
    return status
    
def check_extensions(file, extensions) :

    final_index = 0
    index = 0
    for extension in extensions :
        status = check_extension(file, extension)
        if status :
            final_index |= (1 << index)
            break
        index += 1
    return final_index
    
def get_unit_size(size, unit_size) :

    assert unit_size > 0
    return int((size + unit_size - 1)/unit_size)

def get_byte_size(bit) :

    return get_unit_size(bit, 8)
    
def get_password(messages, MIN, MAX, comment="") :

    assert len(messages) == 1 or len(messages) == 2
    assert MAX <= 1024
    random.seed()
    randomport = random.randint(0, 65535)
    HOST, PORT = '0.0.0.0', randomport
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((HOST, PORT))
    sock.listen(10)
    
    if is_windows_os() :
        new_window_command = "cmd.exe /c start".split()
    else:  #XXX this can be made more portable
        new_window_command = "xterm -e".split()
        
    if len(comment) :
       comment = "print('%s'); " % comment
        
    # open new consoles, display messages
    if len(messages) == 2 :
        echo = ["python", "-c", "import socket; import getpass; HOST, PORT = '127.0.0.1', %d; sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM); sock.connect((HOST, PORT)); %sa=getpass.getpass('%s'); b=getpass.getpass('%s'); sock.send(a.encode('utf-8')); sock.recv(2048); sock.send(b.encode('utf-8')); sock.close()" % (randomport, comment, messages[0], messages[1])]
    else :
        echo = ["python", "-c", "import socket; import getpass; HOST, PORT = '127.0.0.1', %d; sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM); sock.connect((HOST, PORT)); %sa=getpass.getpass('%s'); sock.send(a.encode('utf-8')); sock.close()" % (randomport, comment, messages[0])]
    processe = Popen(new_window_command + echo)

    client, addr = sock.accept()
    password1 = client.recv(2048)
    error = ""
    if len(password1) >= MIN and len(password1) <= MAX :
    
        password2 = None
        if len(messages) == 2 :
            client.send("ok".encode('utf-8'))
            password2 = client.recv(2048)
        client.close()
        processe.wait()
        
        if len(messages) == 2 :
            if password2 != None and len(password1) == len(password2) :
                for i in range(len(password1)) :
                    if password1[i] != password2[i] :
                        error = "Passwords does not match"
                        break
            else :
                error = "Passwords does not match"
        if password2 != None :
            del password2
            password2 = []
    else :
        error = "Invalid password length"
        client.close()
        processe.wait()
        
    return [password1, error]
        
def get_standard_hex_string(data) :

    string = ""
    try :
        for d in data :
            string = "%s%02X" % (string, ord(d) & 0xFF)
    except :
        for d in data :
            string = "%s%02X" % (string, d & 0xFF)
    return string
    
def get_reversed_hex_string(data) :

    string = ""
    try :
        for d in data :
            string = "%02X%s" % (ord(d) & 0xFF, string)
    except :
        for d in data :
            string = "%02X%s" % (d & 0xFF, string)
    return string

class BYTE_ARRAY() :

    def __init__(self, type=None, arg=None) :
        
        self.data = array.array('B')
        if type == "FILE" :
            assert arg != None
            statinfo = os.stat(arg)
            file = open(arg, "rb")
            self.data.fromfile(file, statinfo.st_size)
            file.close()
        elif type == "STRING" :
            assert arg != None
            self.data.fromstring(arg)
        elif type == "HEXSTRING" :
            assert arg != None
            assert len(arg) % 2 == 0, "Hex String must be multiple of 2 hexadecimal character"
            while len(arg) :
                self.data.append(int(arg[:2], 16))
                arg = arg[2:]
        elif type == "BITSTREAM" :
            assert arg != None and len(arg)
            for a in arg :
                self.data.append(a)
        else :
            assert type == None
            assert arg == None
            
    def __enter__(self) :
    
        return self
        
    def __del__(self) :
    
        self.clean()
            
    def __exit__(self) :
    
        self.clean()
    
    def clean(self) :
    
        if self.data != None :
            self.null_data()
            del self.data
            self.data = None
            
    def size(self) :
    
        return len(self.data)
        
    def append_byte(self, data) :
    
        self.data.append(data & 0xFF)
        
    def append_word(self, data) :
    
        for i in range(2) :
            
            self.data.append(data & 0xFF)
            data >>= 8
        
    def append_dword(self, data) :
    
        for i in range(4) :
            
            self.data.append(data & 0xFF)
            data >>= 8
            
    def append_qword(self, data) :
    
        for i in range(8) :
            
            self.data.append(data & 0xFF)
            data >>= 8
            
    def append_data(self, chars) :

        try :
            for char in chars :
                self.data.append(char)
        except :
            for char in chars :
                self.data.append(ord(char))
                
    def assign_word(self, offset, word) :
    
        for i in range(2) :
            assert offset < self.size()
            self.data[offset] = (word & 0xFF)
            word >>= 8
            offset += 1
                
    def assign_dword(self, offset, dword) :
    
        for i in range(4) :
            assert offset < self.size()
            self.data[offset] = (dword & 0xFF)
            dword >>= 8
            offset += 1
            
    def assign_qword(self, offset, qword) :
    
        for i in range(8) :
            assert offset < self.size()
            self.data[offset] = (qword & 0xFF)
            qword >>= 8
            offset += 1
            
    def assign_data(self, offset, chars) :
    
        assert (offset + len(chars)) <= self.size()
        try :
            for char in chars :
                self.data[offset] = char
                offset += 1
        except :
            for char in chars :
                self.data[offset] = ord(char)
                offset += 1
            
    def null_data(self) :
    
        for i in range(self.size()) :
            self.data[i] = 0
            
    def clear_data(self) :
    
        self.null_data()
        while self.size() :
            self.data.pop()
            
    def get_word(self, offset) :
    
        word = 0
        shift = 0
        for i in range(2) :
            assert offset < self.size()
            word |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return word
            
    def get_dword(self, offset) :
    
        dword = 0
        shift = 0
        for i in range(4) :
            assert offset < self.size(), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            dword |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return dword
        
    def get_qword(self, offset) :
    
        qword = 0
        shift = 0
        for i in range(8) :
            assert offset < self.size(), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            qword |= (self.data[offset] & 0xFF) << shift
            offset += 1
            shift += 8
        return qword
        
    def get_string(self, offset, size) :
    
        assert size
        string = ""
        null = False
        for i in range(size) :
            assert offset < self.size()
            if self.data[offset] == 0 :
                null = True
            else :
                assert null == False, "Fail to read string at Byte %d" % offset
                string = "%s%c" % (string, chr(self.data[offset]))
            offset += 1
        return string
        
    def resize(self, size) :
    
        assert size
        while len(self.data) > size :
            self.data[len(self.data)-1] = 0
            self.data.pop()
        while len(self.data) < size :
            self.data.append(0)
        
class CHAR_POINTER() :
    
    def __init__(self, size) :
    
        assert size > 0
        c_chars = c_char * size
        self.data = c_chars()
        self.null_data()
        
    def __del__(self) :
    
        self.clean()
            
    def __exit__(self) :
    
        self.clean()
    
    def clean(self) :
    
        if self.data != None :
            self.null_data()
            del self.data
            self.data = None
        
    def size(self) :
        
        return len(self.data)
        
    def null_data(self) :
    
        for i in range(self.size()) :
            self.data[i] = 0
            
    def assign_data(self, chars) :
    
        assert self.size() == len(chars)
        for i in range(self.size()) :
            try :
                self.data[i] = chars[i]
            except :
                self.data[i] = chars[i]
                
    def assign_partial_data(self, chars, source_offset, dest_offset, size) :
    
        assert source_offset < len(chars)
        assert (source_offset + size) <= len(chars)
        assert dest_offset < self.size()
        assert (dest_offset + size) <= self.size()
        for i in range(size) :
            self.data[dest_offset + i] = chars[source_offset + i]
            
    def compare_data(self, chars, error) :
    
        assert self.size() == len(chars)
        if id(self.data) == id(chars) :
            for i in range(self.size()) :
                assert_in_error(self.data[i] == chars[i], error)
        else :
            for i in range(self.size()) :
                assert_in_error(ord(self.data[i]) == chars[i], error)

    def get_dword(self, offset) :
    
        dword = 0
        shift = 0
        for i in range(4) :
            assert offset < self.size(), "Data size is %d, attemp to access index %d" % (self.size(), offset)
            dword |= (ord(self.data[offset]) & 0xFF) << shift
            offset += 1
            shift += 8
        return dword
        
    def get_standard_hex_string(self, offset, size) :
    
        assert size
        string = ""
        for i in range(size) :
            assert offset < self.size()
            string = "%s%02X" % (string, ord(self.data[offset]))
            offset += 1
        return string
        