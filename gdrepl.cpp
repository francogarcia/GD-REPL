/* gdrepl.cpp */

#include "gdrepl.h"

//#ifdef GDSCRIPT_ENABLED
#include "modules/gdscript/gd_script.h"

static String _disassemble_addr(const Ref<GDScript>& p_script, const GDFunction& func, int p_addr, const Vector<String>& p_code);
static void _dissassemble_function(const Ref<GDScript>& p_class, const GDFunction& p_function, const Vector<String> p_code);
static void _disassemble_class(const Ref<GDScript>& p_class, const Vector<String>& p_code);
#include "gdrepldisassemble.impl"

void REPL::_bind_methods() {

	ObjectTypeDB::bind_method("load_file", &REPL::load_file);
	ObjectTypeDB::bind_method("reload", &REPL::reload);

	ObjectTypeDB::bind_method("eval", &REPL::eval);
	ObjectTypeDB::bind_method("eval_expression", &REPL::eval_expression);
	ObjectTypeDB::bind_method("eval_code_block", &REPL::eval_code_block);

	ObjectTypeDB::bind_method("print_subclasses", &REPL::print_subclasses);
	ObjectTypeDB::bind_method("print_constants", &REPL::print_constants);
	ObjectTypeDB::bind_method("print_members", &REPL::print_members);
	ObjectTypeDB::bind_method("print_member_functions", &REPL::print_member_functions);
	ObjectTypeDB::bind_method("print_member_function_code", &REPL::print_member_function_code);
}

REPL::REPL() {

	//GDScript gdScript;
	//m_pScriptLanguage = gdScript.get_language();
	m_pScriptLanguage = GDScriptLanguage::get_singleton();

	// Loaded script.
	m_pScript = Ref<Script>(m_pScriptLanguage->create_script());
}

REPL::~REPL() {

	m_pScriptLanguage = NULL;
	//m_pScript = NULL;
}

Error REPL::load_file(const String& p_filepath) {

	Error error = static_cast<Ref<GDScript> >(m_pScript)->load_source_code(p_filepath);
	if (error != OK) {

		return error;
	}

	return reload();
}

Error REPL::reload() {

	return m_pScript->reload();
}

Variant REPL::eval(const String& p_expression) {

	return run_script_code("\treturn " + (p_expression.strip_edges()));
}

Variant REPL::eval_expression(const String& p_expression) {

	return run_script_code("\treturn " + (p_expression.strip_edges()));
}

Variant REPL::eval_code_block(const String& p_code_block) {

	String script_text = "";

	// Append a tab for every line of the code block.
	Vector<String> lines = p_code_block.strip_edges().split("\n");
	int total_lines = lines.size();
	for (int i = 0; i < total_lines; i++) {

		script_text += "\t" + lines[i] + "\n";
	}

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

//#endif // GDSCRIPT_ENABLED
