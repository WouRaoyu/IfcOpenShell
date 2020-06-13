#ifndef CITYGML_SEMANTICS_GRIVY
#define CITYGML_SEMANTICS_GRIVY

#include <set>
#include <boost/algorithm/string.hpp>

#include "ifcparse/IfcParse.h"
#include "ifcparse/Ifc2x3.h"

typedef std::map<int,std::map<std::string,std::string>>	semanticMAP;

Ifc2x3::IfcSlabTypeEnum::Value getSlabType(const Ifc2x3::IfcSlab* slab);
Ifc2x3::IfcCoveringTypeEnum::Value getCoveringType(const Ifc2x3::IfcBuildingElement* element);
std::string get_IDstr(const Ifc2x3::IfcObjectDefinition* objectDef);
bool find_semantic(const Ifc2x3::IfcObjectDefinition* objectDef, std::string& sem, semanticMAP sMap,bool canBeWindow=true);
bool determine_buildingElementSemantic(const Ifc2x3::IfcObjectDefinition* objectDef,std::string& sem,semanticMAP smapONLY,semanticMAP smapANY,semanticMAP smapPART,bool canBeWindow=true, std::set<unsigned int> processedIDset=std::set<unsigned int>());
std::string get_semantic(const Ifc2x3::IfcObjectDefinition* objectDef, semanticMAP smapONLY, semanticMAP smapANY,semanticMAP smapPART);
std::vector<semanticMAP> read_semanticSettings();

#endif