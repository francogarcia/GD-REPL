extends Node

const kPi = 3.14159
const kE  = 2.71828

var m_DefaultValue = "This is a string."
var m_Foo

func _ready():
	m_Foo = "Hello, world!"

func print_foo():
	print(m_Foo)

func get_foo():
	return m_Foo

func set_foo(newValue):
	m_Foo = newValue

func get_pi():
	return kPi

func print_constants(bPrintPi, bPrintE):
	if (bPrintPi):
		print(kPi)
	if (bPrintE):
		print(kE)
