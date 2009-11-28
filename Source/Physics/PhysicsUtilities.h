#pragma once

extern bool ConfigureJointItem(const TiXmlElement *element, b2JointDef &joint);

extern void UnpackJointDef(b2JointDef &aDef, unsigned int aId);
