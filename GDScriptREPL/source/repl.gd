extends VBoxContainer


var m_Script = GDScript.new()
var m_SessionData = {}



func _ready():
    clear()


func clear():
    $TextEditInput.text = "extends Node\n\n" + \
                          "var x = 123\n" + \
                          "var y = 321\n" + \
                          "var f = 1.23456\n" + \
                          "var s = \"the quick brown fox jumps over the lazy dog\"\n" + \
                          "var a = [1, 2, 3]\n" + \
                          "var d = {\"key\": 0}\n" + \
                          "var n = null\n" + \
                          "var n2\n" + \
                          "var b = true\n" + \
                          "var reloadCount = 0\n\n" + \
                          "func foo():\n" +  \
                          "\t\t# Enter the code here.\n" + \
                          "\t\tprintt(\"reloadCount\", reloadCount)\n" + \
                          "\t\treloadCount += 1\n" + \
                          "\t\treturn sin(PI / 2.0)"


func eval_string(codeString):
    m_Script.source_code = codeString
    var error = m_Script.reload(true) # TODO Read Godot's source to check how keep_state works.
    if (error != OK):
        $TextEditOutput.text = "[ERROR] Could not reload the input code."
        return

    var instance = m_Script.new()
    if (instance == null):
        $TextEditOutput.text = "[ERROR] Could not instance the input code."
        return

    restore_session_data(instance)

    # print_members(instance)
    # print_methods(instance)

    var result = instance.call("foo")
    if (result == null):
        $TextEditOutput.text = "[INFO] Code either has failed or has not returned any value."
        return

    save_session_data(instance)

    $TextEditOutput.text = str(result)


func save_session_data(instance):
    m_SessionData.clear()
    var properties = instance.get_property_list()
    for property in properties:
        var name = property.name
        var type = property.type
        var usage = property.usage
        var value = instance.get(name)
        if (type == TYPE_NIL): # Although Flag 0 is TYPE_NIL, it seems to be used for all members.
            if (usage == PROPERTY_USAGE_SCRIPT_VARIABLE): # All variables have Flag 8192.
                m_SessionData[name] = value


func restore_session_data(instance):
    var properties = instance.get_property_list()
    for property in properties:
        var name = property.name
        var type = property.type
        var usage = property.usage
        var value = instance.get(name)
        if (name in m_SessionData):
            instance.set(name, m_SessionData[name])


func print_members(instance):
    # printt(instance.x, funcref(instance, "foo").call_func())
    var properties = instance.get_property_list()
    print("Members")
    printt("name", "type", "value", "class name", "hint", "hint string", "usage")
    for property in properties:
        var name = property.name
        var type = property.type
        var className = property.class_name
        var hint = property.hint
        var hintString = property.hint_string
        var usage = property.usage
        var value = instance.get(name)
        if (type == TYPE_NIL): # Although Flag 0 is TYPE_NIL, it seems to be used for all members.
            if (usage == PROPERTY_USAGE_SCRIPT_VARIABLE): # All variables have Flag 8192.
                printt(name, type, value, className, hint, hintString, usage)


func print_methods(instance):
    var methods = instance.get_method_list()
    var total = 0
    print("Methods")
    printt("name", "arguments", "flags")
    for method in methods:
        # args, default_args, flags, id, name, return: (class_name, hint, hint_string, name, type, usage).
        var name = method.name
        var arguments = method.args
        var flags = method.flags
        if (flags & METHOD_FLAG_FROM_SCRIPT): # Flag 64.
            printt(name, arguments, flags)
        # elif (flags & METHOD_FLAG_VIRTUAL):
            # printt(name, arguments, flags)


func _on_ButtonClear_pressed():
    clear()


func _on_ButtonEvaluate_pressed():
    eval_string($TextEditInput.text)
