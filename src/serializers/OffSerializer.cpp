/*
 * File: OffSerializer.cpp
 * Project: SpaceExtraction
 * File Created: Friday, 30th October 2020 4:21:18 pm
 * Author: WouRaoyu
 * Last Modified: Friday, 30th October 2020 7:29:44 pm
 * Modified By: WouRaoyu
 * Copyright 2020 vge lab
 */

#include <boost/algorithm/string/trim_all.hpp>

#include "OffSerializer.h"


OffSerializer::OffSerializer(const std::string& out_filename, const SerializerSettings& settings)
	: off_stream((out_filename).c_str())
	, offx_stream((out_filename.substr(0, out_filename.length()-4) + "x").c_str())
	, offc_stream((out_filename.substr(0, out_filename.length()-4) + "c").c_str())
	, offLine_count(0)
	, vcount_total(1)
	, settings(settings)
	, precision(settings.precision){
	initSemanticSetting();
}

void OffSerializer::initSemanticSetting()
{
	// setting_fixed["IfcWindow"] = "Window";
	// setting_fixed["IfcDoor"] = "Door";

	setting_fixed["IfcSite"] = "Site";
	setting_fixed["IfcRoof"] = "Roof";
	
	setting_fixed["IfcWall"] = "Wall";
	setting_fixed["IfcWallStandardCase"] = "Wall";
	setting_fixed["IfcCurtainWall"] = "Wall";

	setting_fixed["IfcFooting"] = "Ground";

	//setting_fixed["IfcSpace"] = "Closure";
	// setting_fixed["IfcBuildingElementProxy"] = "Install";
	// setting_fixed["IfcRailing"] = "Install";
	// setting_fixed["IfcRamp"] = "Install";
	// setting_fixed["IfcRampFlight"] = "Install";
	// setting_fixed["IfcStair"] = "Install";
	// setting_fixed["IfcStairFlight"] = "Install";
	// setting_fixed["IfcColumn"] = "Install";

	setting_fixed["IfcSlab"] = "Floor";
	// IfcSlab -> unsure maybe floor roof site groud...
	// IfcPlate -> unsure maybe floor roof site groud...
}



std::string OffSerializer::semanticName(std::string type)
{
	return setting_fixed[type];
}

bool OffSerializer::ready() {
	return off_stream.is_open() && offx_stream.is_open() && offc_stream.is_open();
}

void OffSerializer::writeMaterial(const IfcGeom::Material & style)
{

}

void OffSerializer::write(const IfcGeom::TriangulationElement<real_t>* o)
{
	std::string sem_type = semanticName(o->type());
	int element_id = o->id(); // Update this id for parent or aggregate situation

	if (o->type() == "IfcSlab")
	{
		int count = o->product()->data().getArgumentCount();
		std::string type = o->product()->data().getArgument(count - 1)->toString();
		sem_type = type.substr(1, type.size() - 2).c_str();
		if (sem_type == "FLOOR") sem_type = "Floor";
		else if (sem_type == "ROOF") sem_type = "Roof";
		else return;
	}

	if (sem_type.empty())
	{
		IfcUtil::IfcBaseClass* parent = ifc_file->instance_by_id(o->parent_id());
		std::string type = parent->declaration().name();
		sem_type = semanticName(type);
		if (sem_type.empty()) {
			for (std::pair<int*, std::set<int>> agg : aggregate_fixed) {
				if (agg.second.find(o->id()) != agg.second.end()) {
					element_id = agg.first[1];
					if (agg.first[0] == 0) sem_type = "Wall";
					else if (agg.first[0] == 1) sem_type = "Roof";
					break;
				}
			}
		} else {
			element_id = o->parent_id();
		}
		if (sem_type.empty()) return;
	}

	const IfcGeom::Representation::Triangulation<real_t>& mesh = o->geometry();

	unsigned int v_count = 0, f_count = 0;
	std::stringstream vSStream;
	std::stringstream fSStream;

	std::vector<std::vector<double>> vUniqueSet;		//set unsafe, unpredictable ordering
	std::map<int, int> vRefMap;

	const int vcount = (int)mesh.verts().size() / 3;
	for (std::vector<real_t>::const_iterator it = mesh.verts().begin(); it != mesh.verts().end();) {
		const real_t x = *(it++) + (real_t)settings.offset[0];
		const real_t y = *(it++) + (real_t)settings.offset[1];
		const real_t z = *(it++) + (real_t)settings.offset[2];
		
		std::vector<double> vVector;	//put coords in vector
		vVector.push_back(x); vVector.push_back(y); vVector.push_back(z);

		int vRef_count = 0;		// determine count of first vector
		for (std::vector<std::vector<double>>::iterator vIt = vUniqueSet.begin(); vIt != vUniqueSet.end(); vIt++, vRef_count++) {
			if (*vIt == vVector) break;
		}

		vRefMap[v_count] = vRef_count;	// link coords to first its first occurance
		if (vRef_count == vUniqueSet.size()) {	// if new vector
			vUniqueSet.push_back(vVector);		// add vector to set
			vSStream << x << " " << y << " " << z << std::endl;	//print to file
		}
		++v_count;
	}

	for (std::vector<int>::const_iterator it = mesh.faces().begin(); it != mesh.faces().end(); ) {
		const int v1 = *(it++) + vcount_total - 1;
		const int v2 = *(it++) + vcount_total - 1;
		const int v3 = *(it++) + vcount_total - 1;
		fSStream << 3 << " " << vRefMap[v1] << " " << vRefMap[v2] << " " << vRefMap[v3] << std::endl;
		++f_count;
	}

	off_stream << "OFF" << std::endl << vUniqueSet.size() << " " << f_count << " 0" << std::endl;
	off_stream << vSStream.str() << fSStream.str();
	unsigned int next_offLine_count = offLine_count + 2 + vUniqueSet.size() + f_count;

	int storey_id;
	for (std::map<int, std::set<int>>::iterator it = storey_fixed.begin(); it != storey_fixed.end(); it++)
	{
		std::pair<int, std::set<int>> storey_info = *it;
		if (storey_info.second.find(element_id) != storey_info.second.end())
		{
			storey_id = storey_info.first;
		}
	}

	std::string semantics = sem_type + " " + std::to_string(o->id()) + " " + o->type() + " " + std::to_string(storey_id);
	offx_stream << semantics << " " << offLine_count << " " << next_offLine_count << std::endl;
	offLine_count = next_offLine_count;
}

void OffSerializer::finalize()
{
	if (off_stream.is_open()) off_stream.close();
	if (offx_stream.is_open()) offx_stream.close();
	IfcEntityList::ptr connects = ifc_file->instances_by_type("IfcRelConnectsPathElements");
	if (!connects) return;
	for (IfcEntityList::it it = connects.get()->begin(); it != connects.get()->end(); it++)
	{
		IfcUtil::IfcBaseClass* entity = *it;
		std::string connectA = entity->data().getArgument(5)->toString();
		std::string connectB = entity->data().getArgument(6)->toString();
		offc_stream << connectA.substr(1) << " " << connectB.substr(1) << std::endl;
	}
	offc_stream.close();
}

void OffSerializer::generateStorey()
{
	IfcEntityList::ptr storey_list = ifc_file->instances_by_type("IfcBuildingStorey");
	
	std::vector<std::pair<double, int>> storey_preinfo;
	for (IfcEntityList::it it = storey_list.get()->begin(); it != storey_list.get()->end(); it++)
	{
		IfcUtil::IfcBaseClass* entity = *it;
		std::string hgt_str = entity->data().getArgument(9)->toString();
		double height = hgt_str == "$" ? 0.0 : std::stod(hgt_str);
		int id = entity->data().id();
		std::pair<double, int> storey(height, id);
		storey_preinfo.push_back(storey);
	}

	std::sort(storey_preinfo.begin(), storey_preinfo.end()); // 按楼层高度进行排序

	IfcEntityList::ptr structure_list = ifc_file->instances_by_type("IfcRelContainedInSpatialStructure");

	for (IfcEntityList::it it = structure_list.get()->begin(); it != structure_list.get()->end(); it++)
	{
		IfcUtil::IfcBaseClass* entity = *it;
		std::string entities = entity->data().getArgument(4)->toString();
		entities = entities.substr(2, entities.size() - 3); // 去掉头部的 (# 和尾部的 )
		std::vector<std::string> entities_id_str;

		boost::split(entities_id_str, entities, boost::is_any_of(",#"), boost::token_compress_on);
		std::set<int> entities_in_one_storey;
		for (size_t i = 0; i < entities_id_str.size(); i++)
		{
			entities_in_one_storey.emplace(std::stoi(entities_id_str[i]));
		}
		std::string storey = entity->data().getArgument(5)->toString().substr(1);
		int storey_id = std::stoi(storey);
		for (size_t i = 0; i < storey_preinfo.size(); i++)
		{
			std::pair<double, int> info_storey = storey_preinfo[i];
			if (info_storey.second == storey_id) {
				storey_fixed[i] = entities_in_one_storey;
			}
		}
	}
}

void OffSerializer::generateAggregate()
{
	IfcEntityList::ptr aggregate_list = ifc_file->instances_by_type("IfcRelAggregates");

	for (IfcEntityList::it it = aggregate_list.get()->begin(); it != aggregate_list.get()->end(); it++)
	{
		IfcUtil::IfcBaseClass* entity = *it;

		int parent = std::stoi(entity->data().getArgument(4)->toString().substr(1));

		const IfcParse::declaration decl = ifc_file->instance_by_id(parent)->declaration();
		if (!decl.is("IfcCurtainWall") && !decl.is("IfcRoof")) return;
		std::string entities = entity->data().getArgument(5)->toString();
		entities = entities.substr(2, entities.size() - 3); // 去掉头部的 (# 和尾部的 )
		std::vector<std::string> entities_id_str;
		boost::split(entities_id_str, entities, boost::is_any_of(",#"), boost::token_compress_on);

		int index_type; // 0代表wall 1代表roof
		if (decl.is("IfcCurtainWall")) index_type = 0;
		else if (decl.is("IfcRoof")) index_type = 1;
		

		std::set<int> entities_id;
		for (size_t i = 0; i < entities_id_str.size(); i++)
		{
			entities_id.emplace(std::stoi(entities_id_str[i]));
		}
		int index_pair[2] = {index_type, parent};
		aggregate_fixed[index_pair] = entities_id;
	}
}
