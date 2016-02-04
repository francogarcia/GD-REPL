/* register_types.cpp */

#include "register_types.h"
#include "object_type_db.h"

#include "gdrepl.h"

void register_gdrepl_types() {

        ObjectTypeDB::register_type<REPL>();
}

void unregister_gdrepl_types() {
   //nothing to do here
}
