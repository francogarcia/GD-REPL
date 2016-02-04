/* gdrepl.cpp */

#include "gdrepl.h"

// TODO Find a better way to reference the path of an optional module.
#include "../gdscript/gd_script.h"

void REPL::_bind_methods() {

	ObjectTypeDB::bind_method("eval", &REPL::eval);
}

REPL::REPL() {

	//GDScript gdScript;
	//m_pScriptLanguage = gdScript.get_language();
	m_pScriptLanguage = GDScriptLanguage::get_singleton();
}

REPL::~REPL() {

	m_pScriptLanguage = NULL;
}

String REPL::eval(const String& expression) {

	Ref<Script> script = Ref<Script>(m_pScriptLanguage->create_script());
	script->set_source_code(build_script(expression));
	//ERR_FAIL_COND(!script.is_valid());
	Error error = script->reload();
	if (error) {

		print_line("Error: " + expression);
		return "[ERROR Loading creating the script]";
	}

	ScriptInstance* pInstance = script->instance_create(this);
	Variant::CallError callError;
	String result = pInstance->call("s", NULL, 0, callError);
	// memdelete(pInstance); // FIXME Memory leak here.
	if (callError.error == Variant::CallError::CALL_OK) {

		return result;
	}

	print_line("Error eval! Error code: " + itos(callError.error));
	memdelete(pInstance);

	return "[ERROR: Evaluation returned error code:" + itos(callError.error) + "]";
}

// Adapted from marynate's
// String PropertyValueEvaluator::_build_script(const String& p_text),
// available in:
// https://github.com/godotengine/godot/pull/453/files
String REPL::build_script(const String& p_text) {

	String script_text = "tool\nvar this\nfunc set_this(p_this):\n\tthis=p_this\nfunc e():\n\treturn ";
	script_text += p_text.strip_edges();
	script_text += "\n\n";

	// Convert the result to string.
	script_text += "func s():\n\treturn str(e())\n";

	return script_text;
}

// To change spaces to tabs: M-x tabify
// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// indent-tabs-mode: t
// End:
