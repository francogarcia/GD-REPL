/* gdrepl.h */

#ifndef GD_REPL_H
#define GD_REPL_H

#include "map.h"
#include "reference.h"
#include "script_language.h"
#include "ustring.h"
#include "variant.h"

class REPL : public Reference {
	OBJ_TYPE(REPL, Reference);

	ScriptLanguage* m_pScriptLanguage;

	// Script used to store the REPL object's state.
	Ref<Script> m_pScript;

protected:
	static void _bind_methods();

public:
	REPL();
	virtual ~REPL();

	// Load a GDScript file, storing its content in the REPL's internal state.
	Error load_file(const String& p_filepath);

	// Load the script code.
	Error reload();

	Variant eval(const String& p_expression);

	Variant eval_expression(const String& p_expression);
	Variant eval_code_block(const String& p_code_block);

	void print_subclasses() const;
	void print_constants() const;
	void print_members() const;
	void print_member_functions() const;

private:
	// Build a fake tool script to run the expression in a function.
	// The expression is used as the return value of e(), which is called in
	// REPL::eval().
	String build_script(const String& p_text, const bool p_enable_tool_mode = false);

	Variant run_script_code(const String& p_script_code);
};

#endif
