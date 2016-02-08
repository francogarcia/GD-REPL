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

protected:
	static void _bind_methods();

public:
	REPL();
	virtual ~REPL();

	Variant eval(const String& p_expression);

	Variant eval_expression(const String& p_expression);
	Variant eval_code_block(const String& p_code_block);

private:
	// Build a fake tool script to run the expression in a function.
	// The expression is used as the return value of e(), then converted
	// to string in s(), which is called in REPL::eval().
	String build_script(const String& p_text);

	Variant run_script_code(const String& p_script_code);
};

#endif
