#include "Object.h"
#include "ObjectMemoryUtilizer.h"

void Object::deleteLater()
{
	ObjectMemoryUtilizer::instance()->utilize( this );
}