extends VBoxContainer


func _ready():
    clear()


func clear():
    $TextEditInput.text = "extends Node\n\nfunc _ready():\n\t\t# Enter the code here.\n\t\tprint(\"Hello, world!\")\n\t\treturn sin(PI / 2.0)"


func eval_string(codeString):
    var script = GDScript.new()
    script.source_code = codeString
    var error = script.reload()
    if (error != OK):
        $TextEditOutput.text = "[ERROR] Could not reload the input code."
        return

    var instance = script.new()
    if (instance == null):
        $TextEditOutput.text = "[ERROR] Could not instance the input code."
        return

    var result = instance.call("_ready")
    if (result == null):
        $TextEditOutput.text = "[INFO] Code either has failed or has not returned any value."
        return

    $TextEditOutput.text = str(result)


func _on_ButtonClear_pressed():
    clear()


func _on_ButtonEvaluate_pressed():
    eval_string($TextEditInput.text)
