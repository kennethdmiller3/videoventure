#pragma once

extern bool ConfigureJointItem(const tinyxml2::XMLElement *element, b2JointDef &joint);

extern void UnpackJointDef(b2JointDef &aDef, unsigned int aId);
