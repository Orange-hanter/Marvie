#include "hal.h"
#include "Lib/tinyxml2/tinyxml2.h"
#include "Core/Assert.h"

using namespace tinyxml2;

int xmlParserTest()
{
	static const char* xml =
		"<information>"
		"	<attributeApproach v='3' />"
		"	<textApproach>"
		"		<v>2</v>"
		"	</textApproach>"
		"</information>";
	auto m0 = chCoreGetStatusX();
	XMLDocument doc;
	doc.Parse( xml );
	auto dm = m0 - chCoreGetStatusX();

	int v0 = 0;
	int v1 = 0;

	XMLElement* attributeApproachElement = doc.FirstChildElement()->FirstChildElement( "attributeApproach" );
	attributeApproachElement->QueryIntAttribute( "v", &v0 );

	XMLElement* textApproachElement = doc.FirstChildElement()->FirstChildElement( "textApproach" );
	textApproachElement->FirstChildElement( "v" )->QueryIntText( &v1 );

	//printf( "Both values are the same: %d and %d\n", v0, v1 );

	bool gg = !doc.Error() && ( v0 == v1 );

	return 0;
}