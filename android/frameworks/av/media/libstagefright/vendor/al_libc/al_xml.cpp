#define LOG_TAG             "AL_XML"
#include <utils/Log.h>
#include "actal_posix_dev.h"
#include "tinyxml.h"
#include "tinystr.h"
/*
 * return due to the xml value
 * */
int actal_get_appxml_bool(const char *attribute) {

	TiXmlDocument doc("/data/data/com.actions.owlplayer/shared_prefs/config.xml");
	if (!doc.LoadFile()) {
		return FALSE;
	}

	TiXmlNode* node = doc.FirstChild("map");
	if (node == NULL) {
		return FALSE;
	}

	TiXmlElement* topElement = node->ToElement();
	TiXmlElement* itemElement = topElement->FirstChildElement();
	while (itemElement != NULL) {
		const char *name = itemElement->Attribute("name");
		if (name == NULL) {
			continue;
		}

		if (strcmp(name, attribute) == 0) {
			const char *value = itemElement->Attribute("value");
			if (value == NULL) {
				return FALSE;
			}

			if (strcmp(value, "true") == 0) {
				return TRUE;
			}

			break;
		}

		itemElement = itemElement->NextSiblingElement();
	}

	return FALSE;
}
