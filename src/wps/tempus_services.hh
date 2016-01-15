/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_SERVICES_HH
#define TEMPUS_SERVICES_HH

namespace WPS {

class PluginListService : public Service {
public:
    PluginListService();
    Service::ParameterMap execute( const ParameterMap& /*input_parameter_map*/ ) const;
};

class ConstantListService : public Service {
public:
    ConstantListService();
    Service::ParameterMap execute( const ParameterMap& input_parameter_map ) const;
};

class SelectService : public Service {
public:
    SelectService();
    Service::ParameterMap execute( const ParameterMap& input_parameter_map ) const;
};

} // WPS namespace

#endif
