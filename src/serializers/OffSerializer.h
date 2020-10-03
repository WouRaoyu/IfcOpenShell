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

#ifndef OffSERIALIZER_H
#define OffSERIALIZER_H

#include <set>
#include <string>
#include <fstream>

#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Transform.hxx>

#include "GeometrySerializer.h"
#include "../ifcgeom_schema_agnostic/IfcGeomRenderStyles.h"

typedef std::map<std::string, std::string> semanticSetting;

class OffSerializer : public GeometrySerializer {
private:
	std::ofstream off_stream; // geometry info
	std::ofstream offx_stream; // semantic info
	std::ofstream offc_stream; // connection info
	semanticSetting setting_fixed;
	std::map<int, std::set<int>> storey_fixed;
	std::map<int, std::set<int>> aggregate_fixed;
	IfcParse::IfcFile* ifc_file;
	unsigned int offLine_count;
	unsigned int vcount_total;
	short precision;

protected:
	void initSemanticSetting();
	std::string semanticName(std::string type);

public:
	OffSerializer(const std::string& out_filename, const SerializerSettings& setting);

	bool ready();
	void writeHeader() {};
	void writeMaterial(const IfcGeom::Material& style);
	void write(const IfcGeom::TriangulationElement<real_t>* o);
	void write(const IfcGeom::BRepElement<real_t>* /*o*/) {}
	void finalize();
	void generateStorey();
	void generateAggregate();
	bool isTesselated() const { return true; }
	void setUnitNameAndMagnitude(const std::string& /*name*/, float /*magnitude*/) {}
	void setFile(IfcParse::IfcFile* file) { 
		ifc_file = file;
		generateStorey(); // 设置file同时生成storey信息
		generateAggregate(); // 设置file同时生成aggregate信息
	}
};

#endif