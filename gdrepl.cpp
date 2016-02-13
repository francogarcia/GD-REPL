/* gdrepl.cpp */

#include "gdrepl.h"

#ifdef GDSCRIPT_ENABLED

static String _disassemble_addr(const Ref<GDScript>& p_script, const GDFunction& func, int p_addr, const Vector<String>& p_code);
static void _dissassemble_function(const Ref<GDScript>& p_class, const GDFunction& p_function, const Vector<String> p_code);
static void _disassemble_class(const Ref<GDScript>& p_class, const Vector<String>& p_code);
#include "gdrepldisassemble.impl"

void REPL::_bind_methods() {

	ObjectTypeDB::bind_method("load_file", &REPL::load_file);
	ObjectTypeDB::bind_method("load_string", &REPL::load_string);
	ObjectTypeDB::bind_method("reload", &REPL::reload);

	ObjectTypeDB::bind_method("eval", &REPL::eval);
	ObjectTypeDB::bind_method("eval_variable", &REPL::eval_variable);
	ObjectTypeDB::bind_method("eval_function_call", &REPL::eval_function_call);

	ObjectTypeDB::bind_method("eval_expression", &REPL::eval_expression);
	ObjectTypeDB::bind_method("eval_code_block", &REPL::eval_code_block);

	ObjectTypeDB::bind_method("print_subclasses", &REPL::print_subclasses);
	ObjectTypeDB::bind_method("print_constants", &REPL::print_constants);
	ObjectTypeDB::bind_method("print_members", &REPL::print_members);
	ObjectTypeDB::bind_method("print_member_functions", &REPL::print_member_functions);
	ObjectTypeDB::bind_method("print_member_function_code", &REPL::print_member_function_code);
}

REPL::REPL() : m_pScriptInstance(NULL),
			   m_pScriptInstanceObject(NULL) {

	//GDScript gdScript;
	//m_pScriptLanguage = gdScript.get_language();
	m_pScriptLanguage = GDScriptLanguage::get_singleton();

	// Loaded script.
	m_pScript = Ref<Script>(m_pScriptLanguage->create_script());
}

REPL::~REPL() {

	m_pScriptLanguage = NULL;
	//m_pScript = NULL;

	if (m_pScriptInstance == NULL) {

		memdelete(m_pScriptInstance);
	}
	if (m_pScriptInstanceObject == NULL) {

		memdelete(m_pScriptInstanceObject);
	}
}

Error REPL::load_file(const String& p_filepath) {

	Error error = static_cast<Ref<GDScript> >(m_pScript)->load_source_code(p_filepath);
	if (error != OK) {

		return error;
	}

	const String base_path = "";
	const bool just_validate = false;
	const String self_path = "";
	const bool for_completion = false;
	error = m_Parser.parse(static_cast<Ref<GDScript> >(m_pScript)->get_source_code(),
						   base_path,
						   just_validate,
						   self_path,
						   for_completion);
	if (error != OK) {

		return error;
	}

	return reload();
}

Error REPL::load_string(const String& p_code) {

	const String base_path = "";
	const bool just_validate = false;
	const String self_path = "";
	const bool for_completion = false;
	Error error = m_Parser.parse(p_code,
								 base_path,
								 just_validate,
								 self_path,
								 for_completion);
	if (error != OK) {

		print_line("[ERROR Failed to parse the input code.]");

		return error;
	}

	// TODO GDCompiler::parse() does not set the source code.
	// Thus, the following code does nothing special.

	// error = m_Compiler.compile(&m_Parser, static_cast<GDScript*>(m_pScript.ptr()));
	// if (error != OK) {

	//	print_line("[ERROR Failed to compile the input code.]");

	//	return error;
	// }

	// Due to the above issue, we just load the source ourselves and reload().
	static_cast<Ref<GDScript> >(m_pScript)->set_source_code(p_code);
	//print_line(String("Source code:\n") + static_cast<Ref<GDScript> >(m_pScript)->get_source_code());

	return reload();
}

Error REPL::reload() {

	Error error = m_pScript->reload();
	if (error != OK) {

		return error;
	}

	String node_type = static_cast<Ref<GDScript> >(m_pScript)->get_node_type();
	// TODO Godot should return the correct value here.
	// However, at this moment, it always returns "".
	if (node_type.empty()) {

		if (m_pScriptInstanceObject) {

			memdelete(m_pScriptInstanceObject);
		}
		if (m_pScriptInstance) {

			memdelete(m_pScriptInstanceObject);
		}

		m_pScriptInstanceObject = memnew(Node);
		m_pScriptInstance = static_cast<Ref<GDScript> >(m_pScript)->instance_create(m_pScriptInstanceObject);
	}

	return OK;
}

Variant REPL::eval(const String& p_code) {

	//return run_script_code("\treturn " + (p_code.strip_edges()));
	if (!m_pScriptInstance) {
		if (!(static_cast<Ref<GDScript> >(m_pScript)->has_source_code())) {

			return "[Error: there is not a script instance yet.]";
		}

		//m_pScriptInstance = static_cast<Ref<GDScript> >(m_pScript)->instance_create(this);
		return "[Error: there is not a script instance yet.]";
	}

	REPLParser parser;
	//const String code = build_script(p_code); // Godot assumes classes when parsing.
	const String code = p_code;
	const String base_path = "";
	const bool just_validate = false;
	const String self_path = "";
	const bool for_completion = false;

	Error error = parser.parse(code,
							   base_path,
							   just_validate,
							   self_path,
							   for_completion);
	if (error != OK) {
		//print_line("[Error: cannot parse the input: " + parser.get_error() + ".]");

		error = parser.parse_expression(code,
										base_path,
										just_validate,
										self_path,
										for_completion);
		if (error != OK) {
			//print_line("[Error: cannot parse the input: " + parser.get_error() + ".]");

			// error = parser.parse_block(code,
			//						   base_path,
			//						   just_validate,
			//						   self_path,
			//						   for_completion);
			// if (error != OK) {

			//	return "[Error: cannot parse the input: " + parser.get_error() + ".]";
			//	//return error;
			// }

			// Try to run as a code block, as everything else failed.
			return eval_code_block(p_code);
		}
	}

	// GDParser -> REPLParser
	const REPLParser::Node* root = parser.get_parse_tree();

	return eval_tree(root);
}

Variant REPL::eval_tree(const REPLParser::Node* p_node) {

	switch (p_node->type) {

	case REPLParser::Node::TYPE_IDENTIFIER: {

		return eval_variable(static_cast<const REPLParser::IdentifierNode*>(p_node)->name);
	} break;

	case REPLParser::Node::TYPE_CONSTANT: {

		return static_cast<const REPLParser::ConstantNode*>(p_node)->value;
	} break;

	case REPLParser::Node::TYPE_LOCAL_VAR: {

		return eval_variable(static_cast<const REPLParser::LocalVarNode*>(p_node)->name);
	} break;

	case REPLParser::Node::TYPE_ARRAY: {

		Vector<Variant> array;

		const Vector<REPLParser::Node*> elements = static_cast<const REPLParser::ArrayNode*>(p_node)->elements;
		for (int i = 0; i < elements.size(); ++i) {

			array.push_back(eval_tree(elements[i]));
		}
		return array;
	} break;

	case REPLParser::Node::TYPE_DICTIONARY: {

		Dictionary dictionary;

		const Vector<REPLParser::DictionaryNode::Pair> elements = static_cast<const REPLParser::DictionaryNode*>(p_node)->elements;
		for (int i = 0; i < elements.size(); ++i) {

			REPLParser::Node* key = elements[i].key;
			REPLParser::Node* value = elements[i].value;
			dictionary[eval_tree(key)] = eval_tree(value);
		}
		return dictionary;
	} break;

	case REPLParser::Node::TYPE_OPERATOR: {

		const REPLParser::OperatorNode* gd_operator = (static_cast<const REPLParser::OperatorNode*>(p_node));

		// Fetch the arguments.
		const Vector<REPLParser::Node*> arguments = gd_operator->arguments;
		Vector<Variant> arguments_values;
		for (int i = 0; i < arguments.size(); ++i) {

			print_line(itos(arguments[i]->type));
			arguments_values.push_back(eval_tree(arguments[i]));
		}

		// Perform the operation.
		Variant result;
		switch (gd_operator->op) {

// Emacs Lisp code to creating the operators case statements (C-x C-e to eval).

// (defun franco/insert_case (value)
//   (progn
//     (insert "\t\tcase REPLParser::OperatorNode::")
//     (insert value)
//     (insert ": {\n\t\t} break;\n\n")))

// (dolist (value '("OP_CALL"
//                  "OP_PARENT_CALL"
//                  "OP_YIELD"
//                  "OP_EXTENDS"
//                  "OP_INDEX"
//                  "OP_INDEX_NAMED"
//                  "OP_NEG"
//                  "OP_NOT"
//                  "OP_BIT_INVERT"
//                  "OP_PREINC"
//                  "OP_PREDEC"
//                  "OP_INC"
//                  "OP_DEC"
//                  "OP_IN"
//                  "OP_EQUAL"
//                  "OP_NOT_EQUAL"
//                  "OP_LESS"
//                  "OP_LESS_EQUAL"
//                  "OP_GREATER"
//                  "OP_GREATER_EQUAL"
//                  "OP_AND"
//                  "OP_OR"
//                  "OP_ADD"
//                  "OP_SUB"
//                  "OP_MUL"
//                  "OP_DIV"
//                  "OP_MOD"
//                  "OP_SHIFT_LEFT"
//                  "OP_SHIFT_RIGHT"
//                  "OP_INIT_ASSIGN"
//                  "OP_ASSIGN"
//                  "OP_ASSIGN_ADD"
//                  "OP_ASSIGN_SUB"
//                  "OP_ASSIGN_MUL"
//                  "OP_ASSIGN_DIV"
//                  "OP_ASSIGN_MOD"
//                  "OP_ASSIGN_SHIFT_LEFT"
//                  "OP_ASSIGN_SHIFT_RIGHT"
//                  "OP_ASSIGN_BIT_AND"
//                  "OP_ASSIGN_BIT_OR"
//                  "OP_ASSIGN_BIT_XOR"
//                  "OP_BIT_AND"
//                  "OP_BIT_OR"
//                  "OP_BIT_XOR")
//                )
//   (franco/insert_case value))

		case REPLParser::OperatorNode::OP_CALL: {

			const Vector<REPLParser::Node*> arguments = gd_operator->arguments;
			//Vector<Variant> argument_values;
			for (int i = 0; i < arguments.size(); ++i) {

				//print_line(itos(arguments[i]->type));
				//return eval_tree(arguments[i]);
			}
		} break;

		case REPLParser::OperatorNode::OP_PARENT_CALL: {

		} break;

		case REPLParser::OperatorNode::OP_YIELD: {

		} break;

		case REPLParser::OperatorNode::OP_EXTENDS: {

		} break;

		case REPLParser::OperatorNode::OP_INDEX: {

		} break;

		case REPLParser::OperatorNode::OP_INDEX_NAMED: {

		} break;

		case REPLParser::OperatorNode::OP_NEG: {

		} break;

		case REPLParser::OperatorNode::OP_NOT: {

		} break;

		case REPLParser::OperatorNode::OP_BIT_INVERT: {

		} break;

		case REPLParser::OperatorNode::OP_PREINC: {

		} break;

		case REPLParser::OperatorNode::OP_PREDEC: {

		} break;

		case REPLParser::OperatorNode::OP_INC: {

		} break;

		case REPLParser::OperatorNode::OP_DEC: {

		} break;

		case REPLParser::OperatorNode::OP_IN: {

		} break;

		case REPLParser::OperatorNode::OP_EQUAL: {

		} break;

		case REPLParser::OperatorNode::OP_NOT_EQUAL: {

		} break;

		case REPLParser::OperatorNode::OP_LESS: {

		} break;

		case REPLParser::OperatorNode::OP_LESS_EQUAL: {

		} break;

		case REPLParser::OperatorNode::OP_GREATER: {

		} break;

		case REPLParser::OperatorNode::OP_GREATER_EQUAL: {

		} break;

		case REPLParser::OperatorNode::OP_AND: {

		} break;

		case REPLParser::OperatorNode::OP_OR: {

		} break;

		case REPLParser::OperatorNode::OP_ADD: {

		} break;

		case REPLParser::OperatorNode::OP_SUB: {

		} break;

		case REPLParser::OperatorNode::OP_MUL: {

		} break;

		case REPLParser::OperatorNode::OP_DIV: {

		} break;

		case REPLParser::OperatorNode::OP_MOD: {

		} break;

		case REPLParser::OperatorNode::OP_SHIFT_LEFT: {

		} break;

		case REPLParser::OperatorNode::OP_SHIFT_RIGHT: {

		} break;

		case REPLParser::OperatorNode::OP_INIT_ASSIGN: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_ADD: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_SUB: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_MUL: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_DIV: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_MOD: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_SHIFT_LEFT: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_SHIFT_RIGHT: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_BIT_AND: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_BIT_OR: {

		} break;

		case REPLParser::OperatorNode::OP_ASSIGN_BIT_XOR: {

		} break;

		case REPLParser::OperatorNode::OP_BIT_AND: {

		} break;

		case REPLParser::OperatorNode::OP_BIT_OR: {

		} break;

		case REPLParser::OperatorNode::OP_BIT_XOR: {

		} break;

		default: {

		} break;
		}

		// return result;
		return "TODO: Operator\n";
	} break;

	case REPLParser::Node::TYPE_FUNCTION: {

		return "TODO: Function\n";
	} break;

	case REPLParser::Node::TYPE_BUILT_IN_FUNCTION: {

		return "TODO: Built-In Function\n";
	} break;

	case REPLParser::Node::TYPE_BLOCK: {

		return "TODO: BLOCK\n";
	} break;

	case REPLParser::Node::TYPE_CONTROL_FLOW: {


		return "TODO: CONTROL FLOW\n";
	} break;

	case REPLParser::Node::TYPE_ASSERT: {


		return "TODO: ASSERT\n";
	} break;

	case REPLParser::Node::TYPE_BREAKPOINT: {


		return "TODO: BREAK POINT\n";
	} break;

	case REPLParser::Node::TYPE_TYPE: {


		return "TODO: TYPE\n";
	} break;

	case REPLParser::Node::TYPE_SELF: {


		return "TODO: SELF\n";
	} break;

	case REPLParser::Node::TYPE_CLASS: {

		return "TODO: CLASS\n";
	};

	case REPLParser::Node::TYPE_NEWLINE:
	default: {

		return "TODO: NEWLINE\n";
	} break;
	}

	return "[ERROR: Could not determine the type]";
}

Variant REPL::eval_variable(const String& p_variable) {

	if (!m_pScriptInstance) {

		return "[ERROR: Script was not defined yet]";
		//return Variant::NIL;
	}

	Variant value;
	if (!m_pScriptInstance->get(p_variable, value)) {

		return "[ERROR: Variable was not found]";
	}

	return value;
}

Variant REPL::eval_function_call(const String& p_function_call) {

	int first_bracket = p_function_call.find("(");
	int last_bracket = p_function_call.find_last(")");
	String parameters = p_function_call.substr(first_bracket + 1, // 1: Skip the bracket.
											   last_bracket - first_bracket - 1);

	// TODO Consider vectors and function calls as arguments.
	Vector<String> arguments_vector = parameters.split(",", false);
	int arguments_count = arguments_vector.size();

	String method = p_function_call.substr(0, first_bracket);

	String args = "Args: ";
	for (int i = 0; i < arguments_count; ++i) {

		args += arguments_vector[i].strip_edges() + ", ";
		// args += eval_expression(arguments_vector[i]) + ", ";
		// args += eval(arguments_vector[i].strip_edges()) + ", ";
	}

	Variant** arguments = NULL;
	if (arguments_count > 0) {

		// TODO Maybe alloc a single array, and just point to the elements using offsets.
		arguments = memnew_arr(Variant*, arguments_count);
		for (int i = 0; i < arguments_count; ++i) {

			arguments[i] = memnew_arr(Variant, 1);
			arguments[i][0] = eval_expression(arguments_vector[i]);
			// TODO arguments[i][0] = eval(arguments_vector[i].strip_edges()) + ", ";
		}
	}

	Variant::CallError callError;
	Variant result;
	result = m_pScriptInstance->call(method, (const Variant**) arguments, arguments_count, callError);

	// Clean-up.
	if (arguments != NULL) {

		for (int i = 0; i < arguments_count; ++i) {

			memdelete_arr(arguments[i]);
		}
		memdelete_arr(arguments);
	}

	if (callError.error != Variant::CallError::CALL_OK) {

		return "[ERROR: Incorrect function call]";
	}

	return result;
}

Variant REPL::eval_function_call_using_node(const REPLParser::FunctionNode* p_node) {

	Vector<StringName> arguments_vector = p_node->arguments;
	int arguments_count = arguments_vector.size();

	const String method = p_node->name;

	StringName args = "Args: ";
	for (int i = 0; i < arguments_count; ++i) {

		print_line(arguments_vector[i]);

		//args += arguments_vector[i].strip_edges() + ", ";
		// args += eval_expression(arguments_vector[i]) + ", ";
		// args += eval(arguments_vector[i].strip_edges()) + ", ";
	}

	return "Soon...";
}

// Variant REPL::eval_function_call_using_node(const REPLParser::BuiltInFunctionNode* p_node) {

//	GDFunctions::Function function = p_node->function;

//	return "Soon..."
// }

Variant REPL::eval_expression(const String& p_expression) {

	return run_script_code("\treturn " + (p_expression.strip_edges()));
}

Variant REPL::eval_code_block(const String& p_code_block) {

	String script_text = "";

	// Append a tab for every line of the code block.
	// Vector<String> lines = p_code_block.strip_edges().split("\n");
	// int total_lines = lines.size();
	// for (int i = 0; i < total_lines; i++) {

	//	script_text += "\t" + lines[i] + "\n";
	// }
	script_text = p_code_block;

	return run_script_code(script_text);
}

// Adapted from marynate's
// String PropertyValueEvaluator::_build_script(const String& p_text),
// available in:
// https://github.com/godotengine/godot/pull/453/files
String REPL::build_script(const String& p_text, const bool p_enable_tool_mode) const {
	const String empty_lines = "\n\n";

	String script_text = "";
	if (p_enable_tool_mode) {

		script_text += "tool" + empty_lines;
	}
	script_text += "func e():\n";
	//script_text += p_text.strip_edges();
	script_text += p_text;
	script_text += empty_lines;

	//print_line(script_text);

	return script_text;
}

Variant REPL::run_script_code(const String& p_script_code) {

	Ref<Script> script = Ref<Script>(m_pScriptLanguage->create_script());
	script->set_source_code(build_script(p_script_code));
	//ERR_FAIL_COND(!script.is_valid());
	Error error = script->reload();
	if (error) {

		print_line("Error: " + p_script_code);
		return "[ERROR: Call to reload() failed.]";
	}

	ScriptInstance* pInstance = script->instance_create(this);

	Variant::CallError callError;
	Variant result = pInstance->call("e", NULL, 0, callError);
	// memdelete(pInstance); // FIXME Memory leak here.
	if (callError.error == Variant::CallError::CALL_OK) {

		return result;
	}

	print_line("Error: calling the function returned Error Code: " + itos(callError.error));
	memdelete(pInstance);

	return "[ERROR: Running the code returned Error Code: " + itos(callError.error) + "]";
}

void REPL::print_subclasses() const {

	const Map<StringName, Ref<GDScript> >& subclasses = static_cast<Ref<GDScript> >(m_pScript)->get_subclasses();
	print_line("REPL SUBCLASSES");
	print_line("KEY\tNODE TYPE\t (Size = " + itos(subclasses.size()) + ")");
	for (const Map<StringName, Ref<GDScript> >::Element* pElement = subclasses.front();
		 pElement;
		 pElement = pElement->next()) {
		const StringName& key = pElement->key();
		const Ref<GDScript>& value = pElement->value();

		print_line(String(key) + String("\t") + String(value->get_node_type()));
	}
}

void REPL::print_constants() const {

	const Map<StringName, Variant>& constants = static_cast<Ref<GDScript> >(m_pScript)->get_constants();
	print_line("REPL CONSTANTS");
	print_line("KEY\tVALUE\t (Size = " + itos(constants.size()) + ")");
	for (const Map<StringName, Variant>::Element* pElement = constants.front();
		 pElement;
		 pElement = pElement->next()) {
		const StringName& key = pElement->key();
		const Variant& value = pElement->value();

		//print_line(key.cast_to<String>() + String("\t") + value.cast_to<String>());
		print_line(String(key) + String("\t") + String(value));
	}
}

void REPL::print_members() const {

	const Set<StringName>& members = static_cast<Ref<GDScript> >(m_pScript)->get_members();
	print_line("REPL MEMBERS");
	print_line("KEY\tDEFAULT VALUE\t (Size = " + itos(members.size()) + ")");
	for (const Set<StringName>::Element* pElement = members.front();
		 pElement;
		 pElement = pElement->next()) {
		const StringName& name = pElement->get();

		// Only works when TOOLS_ENABLED is defined.
		Variant default_value;
		if (m_pScript->get_property_default_value(name, default_value)) {

			print_line(String(name) + String("\t") + String(default_value));
		} else {

			print_line(String(name) + String("\t") + String("null"));
		}

		// Variant current value;
		// if (m_pScript->_get(name, value)) { // TODO _get() is protected. Create subclass.

		//	print_line(String(name) + String("\t") + String("null")); //print the element
		// } else {

		//	print_line(String(name) + String("\t") + String(value)); //print the element
		// }
	}
}

void REPL::print_member_functions() const {

	const String kArgumentsSeparator = ", ";

	const Map<StringName, GDFunction>& member_functions = static_cast<Ref<GDScript> >(m_pScript)->get_member_functions();
	print_line("REPL MEMBER FUNCTIONS");
	print_line("KEY\tReturn Type\tArguments\t (Size = " + itos(member_functions.size()) + ")");
	for (const Map<StringName, GDFunction>::Element* pElement = member_functions.front();
		 pElement;
		 pElement = pElement->next()) {
		const StringName& key = pElement->key();
		const GDFunction& value = pElement->value();

		//print_line(String(key) + String("\t") + String(value.get_source_code()));
		int arguments_count = value.get_argument_count();
		String arguments = "";
		for (int i = 0; i < arguments_count; ++i) {

			StringName argument_name = value.get_argument_name(i);
			arguments += String(argument_name) + kArgumentsSeparator;
		}
		if (!arguments.empty()) {
			// Remove the last separator.
			const int kArgumentsSeparatorLength = kArgumentsSeparator.length();
			arguments.erase(arguments.length() - kArgumentsSeparatorLength, kArgumentsSeparatorLength);
		}

		String return_type = "Variant";

		print_line(String(key) + String("\t") + return_type + String("\t") + arguments);
	}
}

void REPL::print_member_function_code(const String& p_function_name) const {

	const Map<StringName, GDFunction>& member_functions = static_cast<Ref<GDScript> >(m_pScript)->get_member_functions();
	const Map<StringName, GDFunction>::Element* pElement = member_functions.find(p_function_name);
	if (pElement) {

		const GDFunction& function = pElement->value();
		//print_line(function.get_source_code());
		Vector<String> codeLines;
		codeLines.push_back(m_pScript->get_source_code());
		_dissassemble_function((Ref<GDScript>) m_pScript, function, codeLines);
	} else {

		print_line(p_function_name + String(": member function not found."));
	}
}

#endif // GDSCRIPT_ENABLED
