### Programmer: PYC ### THIS IS SPECIAL STRING. ONLY EDIT IF YOU KNOW HOW !!!
import sys
import getopt
import terminal
import common_util
    
class ARGUMENT_TYPE :

    STR = 0
    INT = 1
    FLOAT = 2
    BOOL = 3
    
class ARGUMENT :

    def __init__(self, long_name, short_name, type, example, detail, multiple=False) :
    
        self.long_name = long_name.lower()
        self.short_name = short_name.lower()
        self.type = type
        self.example = example
        self.detail = detail
        self.multiple = multiple
        self.value = None
        
class OPERATION :

    def __init__(self, operation, detail, must_arguments, optional_arguments, minimum_file_argument, maximum_file_argument, file_argument, additional_help="") :
    
        self.operation = operation.upper()
        self.detail = detail
        self.must_arguments = []
        for arg in must_arguments :
            self.must_arguments.append(arg.lower())
        self.optional_arguments = []
        for arg in optional_arguments :
            self.optional_arguments.append(arg.lower())
        self.minimum_file_argument = minimum_file_argument
        self.maximum_file_argument = maximum_file_argument
        self.file_argument = file_argument
        self.additional_help = additional_help
        
def get_argument(ARGUMENT_DATABASE, print_help=True, check_operation=True) :

    error = ""
    # Check argument is good (for example make sure help and operation is not in the database)
    if "arguments" in ARGUMENT_DATABASE :
        for argument in ARGUMENT_DATABASE["arguments"] :
            if len(argument.long_name) == 0 or len(argument.long_name) == 1:
                error = "Long name for argument (%s) should be at least two characters" % argument.long_name
                break
            elif argument.long_name[0] == ' ' or argument.long_name[0] == '\t' or argument.long_name[0] == '-' :
                error = "Long name for argument (%s) cannot start with space, tab and \"-\"" % argument.long_name
                break
            elif argument.long_name[-1] == ' ' or argument.long_name[-1] == '\t' or argument.long_name[-1] == '-' :
                error = "Long name for argument (%s) cannot end with space, tab and \"-\"" % argument.long_name
                break
            elif argument.long_name == "help" or argument.long_name == "operation" or argument.long_name == "no_color" :
                error = "\"help\", \"operation\" and \"no_color\" are reserved. Cannot use them as long name for argument"
                break
            elif len(argument.short_name) != 0 and len(argument.short_name) != 1 :
                error = "Short name should be 1 character, illegal short name (%s) for argument \"%s\"" % (argument.short_name, argument.long_name)
                break
            elif argument.short_name == "h" or argument.short_name == "o" :
                error = "\"h\", and \"o\" are reserved. Cannot use them as short name for argument \"%s\"" % argument.long_name
                break
                
    if len(error) :
        common_util.print_error(error)
        sys.exit(-1)
        
    # Check if all the operation must/optional argument is good
    if "operations" not in ARGUMENT_DATABASE :
        common_util.print_error("Operation(s) is not defined")
        sys.exit(-1)
        
    operations = []
    for operation in ARGUMENT_DATABASE["operations"] :
        if operation.operation == "" or operation.operation in operations :
            common_util.print_error("Operation \"%s\" is detected more than one or having empty string" % operation.operation)
            sys.exit(-1)
        operations.append(operation.operation)
        arguments = []
        check_operation_arguments(operation.operation, operation.must_arguments, arguments, ARGUMENT_DATABASE)
        check_operation_arguments(operation.operation, operation.optional_arguments, arguments, ARGUMENT_DATABASE)
    
    # Putting internal argument
    arguments = [ARGUMENT("operation", "o", ARGUMENT_TYPE.STR, "operation", "Internal argument to specify operation")]
    if "arguments" in ARGUMENT_DATABASE :
        for argument in ARGUMENT_DATABASE["arguments"] :
            arguments.append(argument)
    arguments.append(ARGUMENT("help", "h", ARGUMENT_TYPE.BOOL, "", "Internal argument to specify help"))
    arguments.append(ARGUMENT("no_color", "o", ARGUMENT_TYPE.BOOL, "", "Internal argument to specify no_color"))
    
    # Parsing the sys argument the argument using getopt
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 
                                        get_short_argument(arguments),           #  Short argument
                                        get_long_argument(arguments))            #  Long argument
    
    except getopt.error as error :
        common_util.print_error(error)
        sys.exit(-1)
        
    # Further parsing the argument
    parse_argument(opts, arguments)
    
    if arguments[-1].value != None and arguments[-1].value :
        terminal.set_no_color()
        
    if arguments[0].value != None :
        arguments[0].value = arguments[0].value.upper()
        
    if print_help and arguments[-2].value != None and arguments[-2].value :
        print_helping_menu(ARGUMENT_DATABASE, arguments[0].value)
        
    if arguments[-2].value == None or arguments[-2].value == False :
        if check_operation :
            if arguments[0].value == None :
                common_util.print_error("Operation is not specified")
                sys.exit(-1)
            else :
                operation = None
                for o in ARGUMENT_DATABASE["operations"] :
                    if o.operation == arguments[0].value :
                        operation = o
                        break
                if operation == None :
                    common_util.print_error("Operation \"%s\" is not supported" % arguments[0].value)
                    sys.exit(-1)
                for arg in operation.must_arguments :
                    for argument in arguments :
                        if argument.long_name == arg :
                            if argument.value == None :
                                common_util.print_error("Incorrect command format")
                                common_util.print_error("Operation \"%s\" must specify argument \"%s\"" % (operation.operation, arg))
                                common_util.print_error(get_operation_format(operation, ARGUMENT_DATABASE["arguments"]))
                                sys.exit(-1)
                            break
                if operation.minimum_file_argument > len(args) :
                    common_util.print_error("Incorrect command format")
                    common_util.print_error("Operation \"%s\" required minumum of %d file argument(s), but only %d argument(s) is specified" % (operation.operation, operation.minimum_file_argument, len(args)))
                    common_util.print_error(get_operation_format(operation, ARGUMENT_DATABASE["arguments"]))
                    sys.exit(-1)
                if operation.maximum_file_argument != -1 and operation.maximum_file_argument < len(args) :
                    common_util.print_error("Incorrect command format")
                    common_util.print_error("Operation \"%s\" required maximum of %d file argument(s), but %d argument(s) is specified" % (operation.operation, operation.maximum_file_argument, len(args)))
                    common_util.print_error(get_operation_format(operation, ARGUMENT_DATABASE["arguments"]))
                    sys.exit(-1)
    values = []
    for i in range(len(arguments) - 1) :
        values.append(arguments[i].value)
    values.append(args)
    return values
    
def check_operation_arguments(operation, arguments, tracking, ARGUMENT_DATABASE) :

    for arg in arguments :
        if arg in tracking :
            common_util.print_error("Operation \"%s\" argument \"%s\" is detected more than one time" % (operation, arg))
            sys.exit(-1)
        tracking.append(arg)
        if "arguments" in ARGUMENT_DATABASE :
            found = False
            for argument in ARGUMENT_DATABASE["arguments"] :
                if argument.long_name == arg :
                    found = True
                    break
            if found == False :
                common_util.print_error("Operation \"%s\" argument \"%s\" is not defined in the list" % (operation, arg))
                sys.exit(-1)
        else :
            common_util.print_error("Argument is not defined. However operation \"%s\" expects to support argument \"%s\"" % (operation, arg))
            sys.exit(-1)
        
def get_short_argument(arguments) :

    arg = ""
    for argument in arguments :
        if argument.short_name != "" :
            arg += argument.short_name
            if argument.type != ARGUMENT_TYPE.BOOL :
                arg += ":"
    return arg

def get_long_argument(arguments) :

    arg = []
    for argument in arguments :
        if argument.type == ARGUMENT_TYPE.BOOL :
            arg.append("%s" % argument.long_name)
        else :
            arg.append("%s=" % argument.long_name)
    return arg
    
def parse_argument(opts, arguments) :

    for o, a in opts :
        o = o.lower()
        found = False
        for argument in arguments :
            option = ["--%s" % argument.long_name]
            if argument.short_name != "" :
                option.append("-%s" % argument.short_name)
            if o in option :
                value = None
                if argument.type == ARGUMENT_TYPE.STR :
                    value = a
                elif argument.type == ARGUMENT_TYPE.INT :
                    value = int(a, 0)
                elif argument.type == ARGUMENT_TYPE.FLOAT :
                    value = float(a)
                elif argument.type == ARGUMENT_TYPE.BOOL :
                    value = True
                else :
                    common_util.print_error("Argument \"%s\" has unsupported type (%s)" % (argument.long_name, argument.type))
                    sys.exit(-1)
                if argument.multiple :
                    if argument.value == None :
                        argument.value = [value]
                    else :
                        argument.value.append(value)
                else :
                    if argument.value == None :
                        argument.value = value
                    else :
                        common_util.print_error("Argument \"%s\" does not support multiple specifying. But it is specified more than one" % (argument.long_name))
                        sys.exit(-1)
                found = True
                break
                
        if found == False :
            common_util.print_error("Argument \"%s\" is not supported" % o)
            sys.exit(-1)
        
def print_helping_menu(ARGUMENT_DATABASE, operation_arg) :

    if operation_arg == None :
    
        help_string = "Usage: python %s --operation=<operation> <more arguments>\n\n   Available operation(s):" % common_util.get_filename(sys.argv[0])
        for operation in ARGUMENT_DATABASE["operations"] :
            help_string = "%s %s," % (help_string, operation.operation)
        help_string = help_string[:-1]
        help_string = "%s\n\n\nTo learn more about each operation, you can specify --help --operation=<operation>" % help_string
        help_string = "%s\n\n   For example: python %s --help --operation=%s" % (help_string, common_util.get_filename(sys.argv[0]), ARGUMENT_DATABASE["operations"][0].operation)
        terminal.printing(("\n" + help_string + "\n"), terminal.MSG_TYPE.NULL, terminal.BCOLORS.HELP, 0, None)
        
    else :
    
        operation = None
        for o in ARGUMENT_DATABASE["operations"] :
            if o.operation == operation_arg :
                operation = o
                break
        if operation == None :
            common_util.print_error("Operation \"%s\" is not supported" % operation_arg)
            sys.exit(-1)
            
        must_arguments = []
        optional_arguments = []
        for arg in operation.must_arguments :
            for argument in ARGUMENT_DATABASE["arguments"] :
                if argument.long_name == arg :
                    must_arguments.append(argument)
        for arg in operation.optional_arguments :
            for argument in ARGUMENT_DATABASE["arguments"] :
                if argument.long_name == arg :
                    optional_arguments.append(argument)
        print_operation_helping_menu(must_arguments, optional_arguments, operation)
    
def print_operation_helping_menu(must_arguments, optional_arguments, operation) :

    argv = sys.argv
    (width, height) = terminal.get_size()
    command = "Usage: python %s" % common_util.get_filename(argv[0])
    help_string = command
    if width > len (command) and (width - len (command)) > 30 :
        pass
    else :
        width = -1 
        
    string_len = len (help_string)
    longest_arg = 0
    
    # Operation
    (help_string, string_len) = append_help_string(help_string, " --operation=%s" % operation.operation, len (command), string_len, width)
    (help_string, string_len, longest_arg) = append_argument_option_to_help_string(help_string, must_arguments, longest_arg, len (command), string_len, width, True)
    (help_string, string_len, longest_arg) = append_argument_option_to_help_string(help_string, optional_arguments, longest_arg, len (command), string_len, width, False)
    
    if len(operation.file_argument) :
        (help_string, string_len) = append_help_string(help_string, " %s" % operation.file_argument, len (command), string_len, width)
    
    if len(operation.additional_help) :
        help_string += ("\n\n   %s" % operation.additional_help)
    
    help_string += ("\n\nOperation: %s" % operation.detail)
    
    if len(must_arguments) + len(optional_arguments) :
        help_string += "\n\nArguments:\n\n"
        help_string = append_argument_to_help_string(help_string, must_arguments, longest_arg, width)
        help_string = append_argument_to_help_string(help_string, optional_arguments, longest_arg, width)
        
    terminal.printing(("\n" + help_string + "\n"), terminal.MSG_TYPE.NULL, terminal.BCOLORS.HELP, 0, None)
    
def append_help_string(help_string, string, space_length, current_string_length, width) :
    
    if width == -1 :
        # ignore
        help_string += string
    elif current_string_length + len (string) + 1 > width :
        # overflow
        help_string += ("\n%s%s" % (" ".ljust(space_length), string))
        current_string_length = space_length + len (string)
    else :
        help_string += string
        current_string_length += len (string)
        
    return (help_string, current_string_length)
    
def append_argument_option_to_help_string(help_string, arguments, longest_arg, space_length, current_string_length, width, must) :

    for arg in arguments :
        argument_string = ""
        if arg.type == ARGUMENT_TYPE.BOOL :
            argument_string = " --%s" % arg.long_name
        else :
            if must :
                argument_string = " --%s=<" % arg.long_name
            else :
                argument_string = " --%s={" % arg.long_name
            if len (arg.example) == 0 :
                argument_string = "%s%s" % (argument_string, arg.long_name)
            else :
                argument_string = "%s%s" % (argument_string, arg.example)
            if must :
                argument_string = "%s>" % argument_string
            else :
                argument_string = "%s}" % argument_string

        (help_string, current_string_length) = append_help_string(help_string, argument_string, space_length, current_string_length, width)
        if len (arg.long_name) > longest_arg :
            longest_arg = len (arg.long_name)
            
    return (help_string, current_string_length, longest_arg)
    
def append_argument_to_help_string(help_string, arguments, longest_arg, width) :
    
    for arg in arguments :
        if len(arg.short_name) :
            argument_string = ("   %s (%s) : " % (arg.long_name.rjust(longest_arg), arg.short_name))
        else :
            argument_string = ("   %s     : " % (arg.long_name.rjust(longest_arg)))
        function = arg.detail
        string_len = len (argument_string)
        help_string += argument_string
        if width != -1 and (len(function) + string_len) > width :
            function = function.split()
            help_string = help_string[:-1]
            string_len -= 1
            for i in range(len(function)) :
                if (len(function[i]) + string_len + 1) > width :
                    help_string += ("\n%s" % " ".ljust(len (argument_string)-1))
                    string_len = len (argument_string) - 1 
                help_string += " %s" % function[i]
                string_len += (1 + len(function[i]))
        else :
            help_string += function
        help_string += "\n"
        
    return help_string
    
def get_operation_format(operation, arguments) :

    (width, height) = terminal.get_size()
    command = "Format: python %s" % common_util.get_filename(sys.argv[0])
    help_string = command
    if width > len (command) and (width - len (command)) > 30 :
        pass
    else :
        width = -1
    width = -1
    must_arguments = []
    optional_arguments = []
    for arg in operation.must_arguments :
        for argument in arguments :
            if argument.long_name == arg :
                must_arguments.append(argument)
    for arg in operation.optional_arguments :
        for argument in arguments :
            if argument.long_name == arg :
                optional_arguments.append(argument)
    string_len = 11 + len(command)
    longest_arg = 0
    (help_string, string_len) = append_help_string(help_string, " --operation=%s" % operation.operation, 11, string_len, width)
    (help_string, string_len, longest_arg) = append_argument_option_to_help_string(help_string, must_arguments, longest_arg, 11, string_len, width, True)
    (help_string, string_len, longest_arg) = append_argument_option_to_help_string(help_string, optional_arguments, longest_arg, 11, string_len, width, False)
    if len(operation.file_argument) :
        (help_string, string_len) = append_help_string(help_string, " %s" % operation.file_argument, 11, string_len, width)
    return help_string
