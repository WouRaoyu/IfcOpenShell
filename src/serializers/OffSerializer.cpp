/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include "OffSerializer.h"

OffSerializer::OffSerializer(const std::string& out_filename, const SerializerSettings& settings)
	: GeometrySerializer(settings)
	, off_stream((out_filename).c_str())
	, offx_stream((out_filename.substr(0, out_filename.length()-4) + "x").c_str())
	, offc_stream((out_filename.substr(0, out_filename.length()-4) + "c").c_str())
	, offLine_count(0)
	, vcount_total(1)
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

	setting_fixed["IfcSpace"] = "Closure";

	// setting_fixed["IfcBuildingElementProxy"] = "Install";
	// setting_fixed["IfcRailing"] = "Install";
	// setting_fixed["IfcRamp"] = "Install";
	// setting_fixed["IfcRampFlight"] = "Install";
	// setting_fixed["IfcStair"] = "Install";
	// setting_fixed["IfcStairFlight"] = "Install";
	// setting_fixed["IfcColumn"] = "Install";

	setting_fixed["IfcSlab"] = "Anything";

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
	if (sem_type.empty())
	{
		IfcUtil::IfcBaseClass* parent = ifc_file->instance_by_id(o->parent_id());
		std::string type = parent->declaration().name();
		sem_type = semanticName(type);
		// if (sem_type.empty()) sem_type = "Anything";
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
		const real_t x = *(it++) + (real_t)settings().offset[0];
		const real_t y = *(it++) + (real_t)settings().offset[1];
		const real_t z = *(it++) + (real_t)settings().offset[2];
		
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

	std::string semantics = sem_type + " " + std::to_string(o->id()) + " " + o->type();
	offx_stream << semantics << " " << offLine_count << " " << next_offLine_count << std::endl;
	offLine_count = next_offLine_count;
}

void OffSerializer::finalize()
{
	IfcEntityList::ptr connects = ifc_file->instances_by_type("IfcRelConnectsPathElements");
	if (!connects) return;
	for (IfcEntityList::it it = connects.get()->begin(); it != connects.get()->end(); it++)
	{
		IfcUtil::IfcBaseClass* entity = *it;
		std::string connectA = entity->data().getArgument(5)->toString();
		std::string connectB = entity->data().getArgument(6)->toString();
		offc_stream << connectA.substr(1, connectA.length()) << " " << connectB.substr(1, connectB.length()) << std::endl;
	}
}
