extends EditorPlugin

# Constants


# Attributes

var m_REPL = null

var m_Variables = null
var m_Functions = null

# Methods

func _ready():
    init()
    set_focus_to_code_input()

    pass


func init():
    m_REPL = REPL.new()

    m_Variables = {}
    m_Functions = {}

    get_node("VBoxContainer/CodeText").set_text("")
    get_node("VBoxContainer/OutputText").set_text("")


func run():
    repl_loop()


func repl_read():
    get_node("VBoxContainer/CodeText").get_text()


func repl_eval(expression):
    var result = m_REPL.eval(expression)

    return result


func repl_print(results):
    get_node("VBoxContainer/OutputText").set_text(results)


func repl_loop(expression):
    #while (true):
        #var expression = repl_read()
    var results = repl_eval(expression)
    repl_print(results)


func set_focus_to_code_input():
    get_node("VBoxContainer/CodeText").grab_focus()


func _on_Button_pressed():
    repl_loop(repl_read())
