#pragma warning(disable : 4819)
#include "CityGMLSemantics.h"

Ifc2x3::IfcSlabTypeEnum::Value getSlabType(const Ifc2x3::IfcSlab* slab) {
	if (slab->hasPredefinedType())										// Check if it has a predefined type
		return slab->PredefinedType();

	Ifc2x3::IfcRelAssociates::list::ptr relList = slab->HasAssociations();	// Check if a type is associated
	for (Ifc2x3::IfcRelAssociates::list::it it = relList->begin(); it != relList->end();++it) {
		if ((*it)->Class().is(Ifc2x3::IfcRelDefinesByType::Class())) {
			Ifc2x3::IfcRelDefinesByType *relDefType = (*it)->as<Ifc2x3::IfcRelDefinesByType>();
			Ifc2x3::IfcTypeObject* type = relDefType->RelatingType();
			if (type->Class().is(Ifc2x3::IfcSlabType::Class())) {
				return type->as<Ifc2x3::IfcSlabType>()->PredefinedType();
	}	}	}	
	return Ifc2x3::IfcSlabTypeEnum::IfcSlabType_NOTDEFINED;
}

Ifc2x3::IfcCoveringTypeEnum::Value getCoveringType(const Ifc2x3::IfcBuildingElement* element) {
	if (element->Class().is(Ifc2x3::IfcCovering::Class())) {
		const Ifc2x3::IfcCovering *covering = element->as<Ifc2x3::IfcCovering>(); 
		if (covering->hasPredefinedType())
			return covering->PredefinedType();
	}
	Ifc2x3::IfcRelAssociates::list::ptr relList = element->HasAssociations();	// Check if a type is associated
	for (Ifc2x3::IfcRelAssociates::list::it it = relList->begin(); it != relList->end(); ++it) {
		if ((*it)->Class().is(Ifc2x3::IfcRelDefinesByType::Class())) {
			Ifc2x3::IfcRelDefinesByType *relDefType = (*it)->as<Ifc2x3::IfcRelDefinesByType>();
			Ifc2x3::IfcTypeObject* type = relDefType->RelatingType();
			if (type->Class().is(Ifc2x3::IfcCoveringType::Class())) {
				return type->as<Ifc2x3::IfcCoveringType>()->PredefinedType();
			}	
		}	
	}
	return Ifc2x3::IfcCoveringTypeEnum::IfcCoveringType_NOTDEFINED;
}

std::string get_IDstr(const Ifc2x3::IfcObjectDefinition* objectDef) {
	std::stringstream idSS;
	idSS << objectDef->GlobalId();
	return idSS.str();
}

bool find_semantic(const Ifc2x3::IfcObjectDefinition* objectDef, std::string& sem, semanticMAP sMap,bool canBeWindowDoor) {
	if (objectDef->Class().is(Ifc2x3::IfcSpace::Class())) {					// Ignore exterior Spaces
		const Ifc2x3::IfcSpace *space = objectDef->as<Ifc2x3::IfcSpace>();
		if (space->InteriorOrExteriorSpace() ==Ifc2x3::IfcInternalOrExternalEnum::IfcInternalOrExternal_EXTERNAL)
			return false;
	}

	std::map<std::string,std::string>::const_iterator it;						
	// Search for types TODO
	it = sMap[-1].find(objectDef->Class().name());
	bool found = it != sMap[-1].end();

	// Search for PreDefinedTypes
	std::map<int,std::map<std::string,std::string>>::const_iterator intmapIt = sMap.find(objectDef->Class().type());
    if (intmapIt != sMap.end()) {
		if (objectDef->Class().is(Ifc2x3::IfcSlab::Class())) {							// Slab
			const Ifc2x3::IfcSlab *slab = objectDef->as<Ifc2x3::IfcSlab>();
			it = intmapIt->second.find(Ifc2x3::IfcSlabTypeEnum::ToString(getSlabType(slab)));
		} else if (objectDef->Class().is(Ifc2x3::IfcCovering::Class())) {				// Covering
			const Ifc2x3::IfcCovering *covering = objectDef->as<Ifc2x3::IfcCovering>();
			it = intmapIt->second.find(Ifc2x3::IfcCoveringTypeEnum::ToString(getCoveringType(covering)));
		}
		found |=  it != intmapIt->second.end();
	} 
	if (found) {
		// Set semantic if found
		if (!canBeWindowDoor && (it->second == "Window" || it->second == "Door")) {
			sem = "Wall " + get_IDstr(objectDef);
		}
		else {
			sem = it->second + " " + get_IDstr(objectDef);
		}
	}
	return found;
}

bool determine_buildingElementSemantic(const Ifc2x3::IfcObjectDefinition* objectDef, std::string& sem, 
										semanticMAP smapONLY, semanticMAP smapANY, semanticMAP smapPART,
										bool canBeWindowDoor, std::set<unsigned int> processedIDset) {
	if (processedIDset.find(objectDef->data().id())!=processedIDset.end()) return false;				// Prevent infinite loop when referencing itself
	processedIDset.insert(objectDef->data().id());

	if (find_semantic(objectDef,sem,smapANY,canBeWindowDoor)) return true;												// Check for top-level semantics
	find_semantic(objectDef,sem,smapPART,canBeWindowDoor);																// Check for low-level semantics (Continue looking foor aggregate)
			
	Ifc2x3::IfcRelDecomposes::list::ptr relDecompList = objectDef->Decomposes();
	for (Ifc2x3::IfcRelDecomposes::list::it it = relDecompList->begin(); it != relDecompList->end();++it) {		// Check if this is a part of a bigger object
		if (determine_buildingElementSemantic((*it)->RelatingObject(),sem,smapONLY,smapANY,smapPART,canBeWindowDoor,processedIDset))		// If better semantic found
			return true;
	}
	// Check if part/contained by Site
	if (objectDef->Class().is(Ifc2x3::IfcElement::Class())) {	
		const Ifc2x3::IfcElement *element = objectDef->as<Ifc2x3::IfcElement>();
		Ifc2x3::IfcRelContainedInSpatialStructure::list::ptr relContainedList = element->ContainedInStructure();
		for (Ifc2x3::IfcRelContainedInSpatialStructure::list::it it = relContainedList->begin(); it != relContainedList->end(); ++it) {
			if (find_semantic((*it)->RelatingStructure(),sem,smapONLY,canBeWindowDoor))		// If better semantic found
			return true;
		}
	}
	return false;
}

std::string get_semantic(const Ifc2x3::IfcObjectDefinition* objectDef, semanticMAP smapONLY, semanticMAP smapANY,semanticMAP smapPART) {		
	std::string sem = "Skip " + get_IDstr(objectDef);				// Global default semantic
	if(!find_semantic(objectDef, sem, smapONLY, true)) {	// Check if in ONLY
		while (objectDef->Class().is(Ifc2x3::IfcBuildingElement::Class())) {							// Check if BuildingElement
			sem = "Anything " + get_IDstr(objectDef);				// BuildingElement default semantic
			bool canBeWindowDoor = true;//objectDef->is(Ifc2x3::IfcPlate::Class());
			std::set<unsigned int> processedIDset;
			determine_buildingElementSemantic(objectDef, sem, smapONLY, smapANY, smapPART, canBeWindowDoor, processedIDset);
			break;
		}	
	}	 
	return sem + " " + objectDef->Class().name();
}


std::vector<semanticMAP> read_semanticSettings() {
	std::vector<semanticMAP> semanticMV;
	std::cerr << "WARNING: Can't open file: IFC2CityGML_Settings.ini" << std::endl;
	std::map<int, std::map<std::string, std::string>> smapONLY;
	std::map<int, std::map<std::string, std::string>> smapANY;
	std::map<int, std::map<std::string, std::string>> smapPART;

	smapONLY[-1]["IfcWindow"] = "Window";
	smapONLY[-1]["IfcDoor"] = "Door";
	smapONLY[-1]["IfcSite"] = "Site";

	smapANY[-1]["IfcRoof"] = "Roof";
	smapANY[-1]["IfcWall"] = "Wall";
	smapANY[-1]["IfcWallStandardCase"] = "Wall";
	smapANY[-1]["IfcCurtainWall"] = "Wall";
	smapANY[-1]["IfcBuildingElementProxy"] = "Install";
	smapANY[-1]["IfcRailing"] = "Install";
	smapANY[-1]["IfcRamp"] = "Install";
	smapANY[-1]["IfcRampFlight"] = "Install";
	smapANY[-1]["IfcStair"] = "Install";
	smapANY[-1]["IfcStairFlight"] = "Install";
	smapANY[Ifc2x3::IfcSlabTypeEnum::IfcSlabType_BASESLAB]["BASESLAB"] = "Ground";
	smapANY[Ifc2x3::IfcSlabTypeEnum::IfcSlabType_LANDING]["LANDING"] = "Install";
	smapPART[Ifc2x3::IfcSlabTypeEnum::IfcSlabType_ROOF]["ROOF"] = "Roof";
	smapPART[Ifc2x3::IfcCoveringTypeEnum::IfcCoveringType_ROOFING]["ROOFING"] = "Roof";

	semanticMV.push_back(smapONLY);
	semanticMV.push_back(smapANY);
	semanticMV.push_back(smapPART);
	return semanticMV;
}
