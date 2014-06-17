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

// Automaton representing forbidden sequences

namespace Tempus {
	
    template <class Entity>	
    class Automaton {
    public:	
		
        struct Node {	
            std::map< int, double > penaltyPerMode; 	
        };
		
        struct Arc {
            Entity entity; // Entity (vertex or edge of the main graph) associated with the automaton edge (or transition)
            int transport_type; // bit field of allowed TransportTypeId 
        };
		
        typedef boost::adjacency_list< boost::vecS, 
                                       boost::vecS, 
                                       boost::directedS,
                                       Node,
                                       Arc > Graph;
	
        typedef typename Graph::vertex_descriptor Vertex; 
        typedef typename Graph::edge_descriptor Edge; 
	
        class NodeWriter
        {
        public:
            NodeWriter( Graph& agraph ) : agraph_(agraph) {}
            void operator() ( std::ostream& ostr, const Vertex& v) const {
                ostr << "[label=\"" << v << " " ;
                for (std::map<int, double>::iterator penaltyIt = agraph_[v].penaltyPerMode.begin(); penaltyIt != agraph_[v].penaltyPerMode.end() ; penaltyIt++ ) {
                    ostr << "(mode:" << penaltyIt->first << ", penalty:" << penaltyIt->second << ") " ;			
                } 			
                ostr << "\"]" ; 
            }
        private:
            Graph& agraph_;
        }; 
		
        class ArcWriter
        {
        public:
            ArcWriter( Graph& agraph, Tempus::Road::Graph& graph ) : agraph_(agraph), graph_(graph) {}
            void operator() ( std::ostream& ostr, const Edge& e) const {
                ostr << "[label=\"" << graph_[agraph_[e].entity].db_id << " = " << agraph_[e].entity << "\"]"; 		
            }
        private:
            Graph& agraph_;
            Tempus::Road::Graph& graph_;
        }; 
		
        // Attributes 	
        Graph automaton_graph_;
        Vertex initial_state_; 
	
        // Constructor
        Automaton() {}
		
        // Methods
        void build_graph( Road::Restrictions sequences )
        {
            automaton_graph_.clear(); 
			
            initial_state_ = boost::add_vertex( automaton_graph_ ); 
	
            for ( std::list<Road::Restriction>::const_iterator it=sequences.restrictions().begin();
                  it!=sequences.restrictions().end();
                  it++ ) {
                add_sequence(it->road_edges(), it->cost_per_transport()); 
            } 
	
            build_shortcuts( build_failure_function() ); // The automaton graph is modified to represent the "next" function, the "failure" function is used to operate this transformation. 
        } 
		
	
        void add_sequence( std::vector< Entity > entity_sequence, std::map< int, double > penaltyPerMode ) // corresponds to adding a sequence in the "goto" function
        {
            Vertex current_state_ = initial_state_ ; 

            // Iterating over forbidden road sequences 
            for ( size_t j = 0; j < entity_sequence.size(); j++ ) {
                std::pair< Vertex, bool > transition_result = find_transition( current_state_, entity_sequence[j] );
				
                if ( transition_result.second == true )  {// this state already exists
                    current_state_ = transition_result.first;
                }
                else { // this state needs to be inserted
                    Vertex s = boost::add_vertex( automaton_graph_ );
                    Edge e;
                    bool ok;
                    boost::tie( e, ok ) = boost::add_edge( current_state_, s, automaton_graph_ );
                    BOOST_ASSERT( ok );
                    automaton_graph_[e].entity = entity_sequence[j];
                    if ( j == entity_sequence.size()-1 ) {
                        automaton_graph_[ s ].penaltyPerMode = penaltyPerMode;
                    }
                    current_state_ = s; 
                } 
            }
        } 
		

        std::map < Vertex, Vertex > build_failure_function()
        {
            std::map< Vertex, Vertex > failure_function_; 
            failure_function_.clear(); 
            std::list< Vertex > q; 
				
            // All successors of the initial state return to initial state when failing
            BGL_FORALL_OUTEDGES_T( initial_state_, edge, automaton_graph_, Graph ) {
                const Vertex& v = target( edge, automaton_graph_ );
                q.push_back( v );
                failure_function_[ v ] = initial_state_;
            } 
				
            // Iterating over the list of states which do not have failure state
            Vertex s, r ; 
            while ( ! q.empty() ) {
                s = q.front();
                q.pop_front();
                BGL_FORALL_OUTEDGES_T( s, edge, automaton_graph_, Graph ) {
                    q.push_back( target( edge, automaton_graph_ ) );
                    r = failure_function_[ s ];
                    while ( find_transition( r, automaton_graph_[edge].entity ).second == false && r != initial_state_ ) {
                        r = failure_function_[ r ];
                    }
                    std::pair<Vertex,bool> t = find_transition( r, automaton_graph_[edge].entity );
                    if ( t.second == true ) {
                        failure_function_[ s ] = t.first;
                    }
                    else {
                        failure_function_[ s ] = initial_state_;
                    }
                } 
            }
			
            return failure_function_; 
        }
			
		
        void build_shortcuts(const std::map < Vertex, Vertex >& failure_function_) // corresponds to building the "next" function
        {
            std::list< Vertex > q; 
			
            BGL_FORALL_OUTEDGES_T( initial_state_, edge, automaton_graph_, Graph ) {
                q.push_back( target( edge, automaton_graph_ ) );
            }

            Vertex r; 
            while ( ! q.empty() ) {
                r = q.front();
                q.pop_front();

                Vertex f = initial_state_;
                typename std::map<Vertex,Vertex>::const_iterator fit = failure_function_.find(r);
                if ( fit != failure_function_.end() ) {
                    f = fit->second;
                }
                if ( f != initial_state_ ) {
                    BGL_FORALL_OUTEDGES_T( f, edge, automaton_graph_, Graph ) {
                        Edge e;
                        bool ok;
                        boost::tie( e, ok ) = boost::add_edge( r, target( edge, automaton_graph_ ), automaton_graph_ );
                        BOOST_ASSERT( ok );
                        automaton_graph_[ e ].entity = automaton_graph_[edge].entity;
                    }
                }
                BGL_FORALL_OUTEDGES_T( r, edge, automaton_graph_, Graph ) {
                    q.push_back( target( edge, automaton_graph_ ) );
                }
            }
        }
		
        std::pair< Vertex, bool > find_transition( Vertex q, Entity e ) const
        {
            BGL_FORALL_OUTEDGES_T( q, edge, automaton_graph_, Graph ) {
                if ( automaton_graph_[ edge ].entity == e ) {
                    Vertex r=target(edge, automaton_graph_);
                    return std::make_pair( target( edge, automaton_graph_ ), true );
                }
            } 
            // If transition not found
            return std::make_pair( 0, false ); 
        }
    }; 
	

	
} // namespace Tempus

