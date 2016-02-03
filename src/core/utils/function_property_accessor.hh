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

#include "function_traits.hpp"

namespace Tempus {
///
/// A FunctionPropertyAccessor implements a Readable Property Map concept
/// by means of a function application on a vertex or edge of a graph
template <class Graph, class Function>
struct FunctionPropertyAccessor {
    using T = typename function_traits<Function>::return_type;
    using value_type = T;
    using reference = const T&;
    using key_type = typename function_traits<Function>::template argument<1>::type;
    using category = boost::readable_property_map_tag;

    FunctionPropertyAccessor( const Graph& graph, Function fct ) : graph_( graph ), fct_( fct ) {}
    const Graph& graph_;
    Function fct_;
};

template <typename Graph, typename Function>
FunctionPropertyAccessor<Graph, Function> make_function_property_accessor( const Graph& graph, Function foo )
{
    return FunctionPropertyAccessor<Graph, Function>( graph, foo );
}

}

namespace boost {
///
/// Implementation of FunctionPropertyAccessor
template <class Graph, class Function>
typename Tempus::FunctionPropertyAccessor<Graph, Function>::value_type get( Tempus::FunctionPropertyAccessor<Graph, Function> pmap, typename Tempus::FunctionPropertyAccessor<Graph, Function>::key_type e )
{
    return pmap.fct_( pmap.graph_, e );
}
}
