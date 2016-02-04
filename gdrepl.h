/* gdrepl.h */

#ifndef GD_REPL_H
#define GD_REPL_H

#include "map.h"
#include "reference.h"
#include "script_language.h"
#include "ustring.h"

class REPL : public Reference {
	OBJ_TYPE(REPL, Reference);

	ScriptLanguage* m_pScriptLanguage;

protected:
	static void _bind_methods();

public:
	REPL();
	virtual ~REPL();

	String eval(const String& expression);

private:
	// Build a fake tool script to run the expression in a function.
	// The expression is used as the return value of e(), then converted
	// to string in s(), which is called in REPL::eval().
	String build_script(const String& p_text);
};

#endif

// To change spaces to tabs: M-x tabify
// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// indent-tabs-mode: t
// End:
