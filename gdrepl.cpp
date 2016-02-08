/* gdrepl.cpp */

#include "gdrepl.h"

// TODO Find a better way to reference the path of an optional module.
#include "../gdscript/gd_script.h"

void REPL::_bind_methods() {

	ObjectTypeDB::bind_method("eval_expression", &REPL::eval_expression);
	ObjectTypeDB::bind_method("eval_code_block", &REPL::eval_code_block);
}

REPL::REPL() {

	//GDScript gdScript;
	//m_pScriptLanguage = gdScript.get_language();
	m_pScriptLanguage = GDScriptLanguage::get_singleton();
}

REPL::~REPL() {

	m_pScriptLanguage = NULL;
}

String REPL::eval_expression(const String& p_expression) {

	return run_script_code("\treturn " + (p_expression.strip_edges()));
}

String REPL::eval_code_block(const String& p_code_block) {

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
String REPL::build_script(const String& p_text) {

	String script_text = "tool\nvar this\nfunc set_this(p_this):\n\tthis=p_this\nfunc e():\n";
	//script_text += p_text.strip_edges();
	script_text += p_text;
	script_text += "\n\n";

	// Convert the result to string.
	script_text += "func s():\n\treturn str(e())\n";

	//print_line(script_text);

	return script_text;
}

String REPL::run_script_code(const String& p_script_code) {

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
	String result = pInstance->call("s", NULL, 0, callError);
	// memdelete(pInstance); // FIXME Memory leak here.
	if (callError.error == Variant::CallError::CALL_OK) {

		return result;
	}

	print_line("Error: calling the function returned Error Code: " + itos(callError.error));
	memdelete(pInstance);

	return "[ERROR: Running the code returned Error Code: " + itos(callError.error) + "]";
}
