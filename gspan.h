#ifndef GSPAN_H_
#define GSPAN_H_

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <stdio.h>
#include <cstring>
#include <iterator>
#include <strstream>

#include <assert.h>
#include "./system/utils/ioser.h"



using namespace std;

template <class T> inline void _swap (T &x, T &y) { T z = x; x = y; y = z; }

template <class T, class Iterator>
void tokenize (const char *str, Iterator iterator)
{
	istrstream is (str, std::strlen(str));
	copy (istream_iterator <T> (is), istream_iterator <T> (), iterator);
}

struct Edge {
	int from;
	int to;
	int elabel;
	unsigned int id;
	Edge(): from(0), to(0), elabel(0), id(0) {};
};

typedef std::vector <Edge*> EdgeList;

//LabelEdge <from.label, e.label, to.label>
struct LabelEdge{
public:
	int fromL;
	int toL;
	int eL;

	LabelEdge(int _fromL = 0, int _toL = 0, int _eL = 0) : fromL(_fromL), toL(_toL), eL(_eL){}

	friend bool operator == (const LabelEdge &o1, const LabelEdge & o2)
	{
		return o1.fromL == o2.fromL && o1.toL == o2.toL && o1.eL == o2.eL;
	}

	friend bool operator < (const LabelEdge &o1, const LabelEdge &o2) {
		if(o1.fromL != o2.fromL) return o1.fromL < o2.fromL;
		else if(o1.toL != o2.toL) return o1.toL < o2.toL;
		else if(o1.eL != o2.eL) return o1.eL < o2.eL;
		return false;
	}

	friend bool operator > (const LabelEdge &o1, const LabelEdge &o2) {
		if(o1.fromL != o2.fromL) return o1.fromL > o2.fromL;
		else if(o1.toL != o2.toL) return o1.toL > o2.toL;
		else if(o1.eL != o2.eL) return o1.eL > o2.eL;
		return false;
	}

	friend ibinstream& operator>>(ibinstream& m, LabelEdge & p)
	{
		m >> p.fromL;
		m >> p.toL;
		m >> p.eL;
    	return m;
	}

	friend obinstream& operator<<(obinstream& m, const LabelEdge & p)
	{
		m << p.fromL;
		m << p.toL;
		m << p.eL;
    	return m;
	}

	friend ifbinstream& operator>>(ifbinstream& m, LabelEdge & p)
	{
		m >> p.fromL;
		m >> p.toL;
		m >> p.eL;
    	return m;
	}

	friend ofbinstream& operator<<(ofbinstream& m, const LabelEdge & p)
	{
		m << p.fromL;
		m << p.toL;
		m << p.eL;
    	return m;
	}
};

class Vertex
{
public:
	typedef std::vector<Edge>::iterator edge_iterator;

	int label;
	std::vector<Edge> edge;

	void push (int from, int to, int elabel)
	{
		edge.resize (edge.size()+1);
		edge[edge.size()-1].from = from;
		edge[edge.size()-1].to = to;
		edge[edge.size()-1].elabel = elabel;
		return;
	}
};

//Transaction type: containing transactions after pruning globally infrequent items;
class Graph: public std::vector<Vertex> {
private:
	unsigned int edge_size_;

public:
	typedef Vertex ItemType;
	typedef typename vector<Vertex>::iterator TIter;

	Graph()
	{
		directed = false;
		edge_size_ = 0;
	};

	Graph (bool _directed)
	{
		directed = _directed;
		edge_size_ = 0;
	};

	bool directed;

	unsigned int edge_size ()
	{
		return edge_size_;
	}

	unsigned int vertex_size ()
	{
		return (unsigned int)size();
	} // wrapper

	void buildEdge (){
		char buf[512];
		std::map <std::string, unsigned int> tmp;

		unsigned int id = 0;
		for (int from = 0; from < (int)size (); ++from) {
			for (Vertex::edge_iterator it = (*this)[from].edge.begin ();
					it != (*this)[from].edge.end (); ++it)
			{
				if (directed || from <= it->to)
					sprintf(buf, "%d %d %d", from, it->to, it->elabel);
				else
					sprintf (buf, "%d %d %d", it->to, from, it->elabel);

				// Assign unique id's for the edges.
				if (tmp.find (buf) == tmp.end()) {
					it->id = id;
					tmp[buf] = id;
					++id;
				} else {
					it->id = tmp[buf];
				}
			}
		}
		edge_size_ = id;
	}
};

class DFS {
public:
	int from;
	int to;
	int fromlabel;
	int elabel;
	int tolabel;
	friend bool operator == (const DFS &d1, const DFS &d2)
	{
		return (d1.from == d2.from && d1.to == d2.to && d1.fromlabel == d2.fromlabel
			&& d1.elabel == d2.elabel && d1.tolabel == d2.tolabel);
	}

	friend bool operator != (const DFS &d1, const DFS &d2) { return (! (d1 == d2)); }

	//used by Grown_Map in SlaveNode, DFS should be compared in an order
	friend bool operator < (const DFS &d1, const DFS &d2) {
		if(d1.from != d2.from) return d1.from < d2.from;
		else if(d1.to != d2.to) return d1.to < d2.to;
		else if(d1.fromlabel != d2.fromlabel) return d1.fromlabel < d2.fromlabel;
		else if(d1.tolabel != d2.tolabel) return d1.tolabel < d2.tolabel;
		else if(d1.elabel != d2.elabel) return d1.elabel < d2.elabel;
		return false;
	}

	friend bool operator > (const DFS &d1, const DFS &d2) {
		if(d1.from != d2.from) return d1.from > d2.from;
		else if(d1.to != d2.to) return d1.to > d2.to;
		else if(d1.fromlabel != d2.fromlabel) return d1.fromlabel > d2.fromlabel;
		else if(d1.tolabel != d2.tolabel) return d1.tolabel > d2.tolabel;
		else if(d1.elabel != d2.elabel) return d1.elabel > d2.elabel;
		return false;
	}

	DFS(): from(0), to(0), fromlabel(0), elabel(0), tolabel(0) {};
	DFS(int _from, int _to, int _fromlabel, int _elabel, int _tolabel){
		from = _from;
		to = _to;
		fromlabel = _fromlabel;
		elabel = _elabel;
		tolabel = _tolabel;
	}

	string print(){
		char buf[512];
		sprintf(buf, "%d %d %d %d %d , ", from, to, fromlabel, elabel, tolabel);
		return buf;
	}

	friend ibinstream& operator>>(ibinstream& m, DFS & p)
	{
		m >> p.from;
		m >> p.to;
		m >> p.fromlabel;
		m >> p.elabel;
		m >> p.tolabel;

    	return m;
	}

	friend obinstream& operator<<(obinstream& m, const DFS & p)
	{
		m << p.from;
		m << p.to;
		m << p.fromlabel;
		m << p.elabel;
		m << p.tolabel;
    	return m;
	}

	friend ifbinstream& operator>>(ifbinstream& m, DFS & p)
	{
		m >> p.from;
		m >> p.to;
		m >> p.fromlabel;
		m >> p.elabel;
		m >> p.tolabel;
    	return m;
	}

	friend ofbinstream& operator<<(ofbinstream& m, const DFS & p)
	{
		m << p.from;
		m << p.to;
		m << p.fromlabel;
		m << p.elabel;
		m << p.tolabel;
    	return m;
	}
};

void hash_combine(size_t & seed, size_t v)
{
  seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std {
  template <>
  struct hash<DFS>
  {
		std::size_t operator()(const DFS& dfs) const
		{
			size_t seed = 0;
			hash_combine(seed, dfs.from);
			hash_combine(seed, dfs.to);
			hash_combine(seed, dfs.fromlabel);
			hash_combine(seed, dfs.elabel);
			hash_combine(seed, dfs.tolabel);
			return  seed;
		}
  };
}

namespace std {
  template <>
  struct hash<LabelEdge>
  {
		std::size_t operator()(const LabelEdge& le) const
		{
			size_t seed = 0;
			hash_combine(seed, le.fromL);
			hash_combine(seed, le.toL);
			hash_combine(seed, le.eL);
			return  seed;
		}
  };
}

typedef vector<int> RMPath;

bool get_forward_root (Graph &g, Vertex &v, EdgeList &result)
{
	result.clear ();
	for (Vertex::edge_iterator it = v.edge.begin(); it != v.edge.end(); ++it) {
		assert (it->to >= 0 && it->to < g.size ());
		if (v.label <= g[it->to].label)
			result.push_back (&(*it));
	}

	return (! result.empty());
}

struct DFSCode: public vector <DFS> {
private:
	RMPath rmpath;
public:
	const RMPath& buildRMPath (){
		rmpath.clear ();

		int old_from = -1;

		for (int i = size() - 1 ; i >= 0 ; --i) {
			if ((*this)[i].from < (*this)[i].to && // forward
					(rmpath.empty() || old_from == (*this)[i].to))
			{
				rmpath.push_back (i);
				old_from = (*this)[i].from;
			}
		}

		return rmpath;
	}

	/* Convert current DFS code into a graph.
	 */
	bool toGraph (Graph & g){
		g.clear ();

		for (DFSCode::iterator it = begin(); it != end(); ++it) {
			g.resize (std::max (it->from, it->to) + 1);

			if (it->fromlabel != -1)
				g[it->from].label = it->fromlabel;
			if (it->tolabel != -1)
				g[it->to].label = it->tolabel;

			g[it->from].push (it->from, it->to, it->elabel);
			if (g.directed == false)
				g[it->to].push (it->to, it->from, it->elabel);
		}

		g.buildEdge ();

		return (true);
	}

	/* Clear current DFS code and build code from the given graph.
	 */
	void fromGraph (Graph &g){
		clear ();

		EdgeList edges;
		for (unsigned int from = 0 ; from < g.size () ; ++from) {
			if (get_forward_root (g, g[from], edges) == false)
				continue;

			for (EdgeList::iterator it = edges.begin () ; it != edges.end () ; ++it)
				push (from, (*it)->to, g[(*it)->from].label, (*it)->elabel, g[(*it)->to].label);
		}
	}

	void push (int from, int to, int fromlabel, int elabel, int tolabel)
	{
		resize (size() + 1);
		DFS &d = (*this)[size()-1];

		d.from = from;
		d.to = to;
		d.fromlabel = fromlabel;
		d.elabel = elabel;
		d.tolabel = tolabel;
	}

	void pop () { resize (size()-1); }
};

class Gpattern:public Pattern<DFS>
{
public:

	Gpattern()
	{
		next = this->begin();
	}

	virtual void grow(const DeltaType & d, Pattern & new_patt)
	{
		for(int i = 0; i < this->size(); i++)
			new_patt.push_back(this->at(i));
		new_patt.push_back(d);

		new_patt.next = new_patt.begin();
	}


	virtual inline void print()
	{
		for(int i = 0; i < this->size(); i++)
			cout<< at(i).print();
	}
};

class PDFS: public ProjTran<Graph, Gpattern>
{
public:
	Edge        *edge;
	PDFS        *prev;
	PDFS(TranType & t, int id):ProjTran(t, id){
		edge = 0;
		prev = 0;
	}

	PDFS(TranType & t, int id, Edge * e, PDFS * p):ProjTran(t, id){
		edge = e;
		prev = p;
	}

	PDFS(int id, Edge * e, PDFS * p){
		tran_id = id;
		edge = e;
		prev = p;
	}
};

class History: public vector<Edge*> {
private:
	vector<int> edge;
	vector<int> vertex;

public:
	bool hasEdge   (unsigned int id) { return (bool)edge[id]; }

	bool hasVertex (unsigned int id) { return (bool)vertex[id]; }

	void build (Graph &graph, PDFS *e)
	{
		// first build history
		clear ();
		edge.clear ();
		edge.resize (graph.edge_size());
		vertex.clear ();
		vertex.resize (graph.size());

		if (e) {
			push_back (e->edge);
			edge[e->edge->id] = vertex[e->edge->from] = vertex[e->edge->to] = 1;

			for (PDFS *p = e->prev ; p ; p = p->prev) {
				push_back (p->edge);	// this line eats 8% of overall instructions(!)
				edge[p->edge->id] = vertex[p->edge->from] = vertex[p->edge->to] = 1;
			}
			std::reverse (begin(), end());
		}
	}

	History() {};
	History (Graph & g, PDFS *p) { build (g, p); }
};

bool get_forward_pure (Graph &graph, Edge *e, int minlabel, History& history, EdgeList &result)
{
	result.clear ();

	assert (e->to >= 0 && e->to < graph.size ());

	/* Walk all edges leaving from vertex e->to.
	 */
	for (Vertex::edge_iterator it = graph[e->to].edge.begin() ;
		it != graph[e->to].edge.end() ; ++it)
	{
		/* -e-> [e->to] -it-> [it->to]
		 */
		assert (it->to >= 0 && it->to < graph.size ());
		if (minlabel > graph[it->to].label || history.hasVertex (it->to))
			continue;

		result.push_back (&(*it));
	}

	return (! result.empty());
}

bool get_forward_rmpath (Graph &graph, Edge *e, int minlabel, History& history, EdgeList &result)
{
	result.clear ();
	assert (e->to >= 0 && e->to < graph.size ());
	assert (e->from >= 0 && e->from < graph.size ());
	int tolabel = graph[e->to].label;

	for (Vertex::edge_iterator it = graph[e->from].edge.begin() ;
		it != graph[e->from].edge.end() ; ++it)
	{
		int tolabel2 = graph[it->to].label;
		if (e->to == it->to || minlabel > tolabel2 || history.hasVertex (it->to))
			continue;

		if (e->elabel < it->elabel || (e->elabel == it->elabel && tolabel <= tolabel2))
			result.push_back (&(*it));
	}

	return (! result.empty());
}

Edge *get_backward (Graph &graph, Edge* e1, Edge* e2, History& history)
{
	if (e1 == e2)
		return 0;

	assert (e1->from >= 0 && e1->from < graph.size ());
	assert (e1->to >= 0 && e1->to < graph.size ());
	assert (e2->to >= 0 && e2->to < graph.size ());

	for (Vertex::edge_iterator it = graph[e2->to].edge.begin() ;
		it != graph[e2->to].edge.end() ; ++it)
	{
		if (history.hasEdge (it->id))
			continue;

		if ( (it->to == e1->from) && ( (e1->elabel < it->elabel) || ((e1->elabel == it->elabel) && (graph[e1->to].label <= graph[e2->to].label))))
		{
			return &(*it);
		}
	}

	return 0;
}

class Projected: public PDB<PDFS>{
public:
	typedef std::map<int, std::map <int, std::map <int, Projected> > >           Projected_map3;
	typedef std::map<int, std::map <int, std::map <int, Projected*> > >         ProjectedP_map3;
	typedef std::map<int, std::map <int, Projected> >                            Projected_map2;
	typedef std::map<int, std::map <int, Projected*> >                            ProjectedP_map2;
	typedef std::map<int, Projected>                                             Projected_map1;
	typedef std::map<int, Projected*>                                             ProjectedP_map1;

	typedef std::map<int, std::map <int, std::map <int, Projected> > >::iterator Projected_iterator3;
	typedef std::map<int, std::map <int, std::map <int, Projected*> > >::iterator ProjectedP_iterator3;
	typedef std::map<int, std::map <int, Projected> >::iterator                  Projected_iterator2;
	typedef std::map<int, std::map <int, Projected*> >::iterator                  ProjectedP_iterator2;
	typedef std::map<int, Projected>::iterator                                   Projected_iterator1;
	typedef std::map<int, Projected*>::iterator                                   ProjectedP_iterator1;
	typedef std::map<int, std::map <int, std::map <int, Projected> > >::reverse_iterator Projected_riterator3;
	typedef std::map<int, std::map <int, std::map <int, Projected*> > >::reverse_iterator ProjectedP_riterator3;

	int _sup;
	bool is_min_dfscode;
	DFSCode dfs_code;

	//calculate whether the DFC_CODE of current graph is MIN_DFS_CODE
	//high compute density
	bool is_min ()
	{
		if (dfs_code.size() == 1)
			return (true);

		DFSCode min_dfs_code;
		Graph min_graph;
		dfs_code.toGraph (min_graph);

		Projected_map3 root;
		EdgeList           edges;

		for (unsigned int from = 0; from < min_graph.size() ; ++from)
			if (get_forward_root (min_graph, min_graph[from], edges))
				for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it)
					root[min_graph[from].label][(*it)->elabel][min_graph[(*it)->to].label].push (0, *it, 0);

		Projected_iterator3 fromlabel = root.begin();
		Projected_iterator2 elabel    = fromlabel->second.begin();
		Projected_iterator1 tolabel   = elabel->second.begin();

		min_dfs_code.push (0, 1, fromlabel->first, elabel->first, tolabel->first);

		return (project_is_min (tolabel->second, min_dfs_code, min_graph));
	}

	bool project_is_min (Projected &projected, DFSCode & min_dfs_code, Graph & min_graph)
	{
		const RMPath& rmpath = min_dfs_code.buildRMPath ();
		int minlabel         = min_dfs_code[0].fromlabel;
		int maxtoc           = min_dfs_code[rmpath[0]].to;

		{
			Projected_map1 root;
			bool flg = false;
			int newto = 0;

			for (int i = rmpath.size()-1; ! flg  && i >= 1; --i) {
				for (unsigned int n = 0; n < projected.size(); ++n) {
					PDFS *cur = projected.projDB[n];
					History history (min_graph, cur);
					Edge *e = get_backward (min_graph, history[rmpath[i]], history[rmpath[0]], history);
					if (e) {
						root[e->elabel].push (0, e, cur);
						newto = min_dfs_code[rmpath[i]].from;
						flg = true;
					}
				}
			}

			if (flg) {
				Projected_iterator1 elabel = root.begin();
				min_dfs_code.push (maxtoc, newto, -1, elabel->first, -1);
				if (dfs_code[min_dfs_code.size()-1] != min_dfs_code [min_dfs_code.size()-1]) return false;
				return project_is_min (elabel->second, min_dfs_code, min_graph);
			}
		}

		{
			bool flg = false;
			int newfrom = 0;
			Projected_map2 root;
			EdgeList edges;

			for (unsigned int n = 0; n < projected.size(); ++n) {
				PDFS *cur = projected.projDB[n];
				History history (min_graph, cur);
				if (get_forward_pure (min_graph, history[rmpath[0]], minlabel, history, edges)) {
					flg = true;
					newfrom = maxtoc;
					for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it)
						root[(*it)->elabel][min_graph[(*it)->to].label].push (0, *it, cur);
				}
			}

			for (int i = 0; ! flg && i < (int)rmpath.size(); ++i) {
				for (unsigned int n = 0; n < projected.size(); ++n) {
					PDFS *cur = projected.projDB[n];
					History history (min_graph, cur);
					if (get_forward_rmpath (min_graph, history[rmpath[i]], minlabel, history, edges)) {
						flg = true;
						newfrom = min_dfs_code[rmpath[i]].from;
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it)
							root[(*it)->elabel][min_graph[(*it)->to].label].push (0, *it, cur);
					}
				}
			}

			if (flg) {
				Projected_iterator2 elabel  = root.begin();
				Projected_iterator1 tolabel = elabel->second.begin();
				min_dfs_code.push (newfrom, maxtoc + 1, -1, elabel->first, tolabel->first);
				if (dfs_code[min_dfs_code.size()-1] != min_dfs_code [min_dfs_code.size()-1]) return false;
				return project_is_min (tolabel->second, min_dfs_code, min_graph);
			}
		}
		return true;
	}

	void comp_support(){
		unsigned int oid = 0xffffffff;
		unsigned int size = 0;

		for (PDBIter cur = projDB.begin(); cur != projDB.end(); ++cur) {
			if (oid != (*cur)->tran_id) {
				++size;
			}
			oid = (*cur)->tran_id;
		}
		_sup =  size;
	}

	//UDF in PDB
	virtual int  support(){
		return _sup;
	}

	//UDF in PDB
	virtual void init(TransactionContainer & db)//should only called by root node initialization
	{
		for(int i = 0; i < db.size(); i++)
		{
			projDB.push_back(new Proj(db[i], i));
		}
		comp_support();
	}

	void push (TranType & t, int tid, Edge *edge, PDFS *prev)
	{
		add(new PDFS(t, tid, edge, prev));
	}

	void push (int tid, Edge *edge, PDFS *prev)
	{
		add(new PDFS(tid, edge, prev));
	}

	//UDF in PDB
	virtual void scan_project (PDB_Map & new_PDB, bool is_root)
	{
		if(is_root){
			EdgeList edges;
			ProjectedP_map3 root;

			for(int i = 0; i < projDB.size(); i++)
			{
				Graph & g = projDB[i]->transaction;
				for (unsigned int from = 0; from < g.size() ; ++from) {
					if (get_forward_root (g, g[from], edges)) {
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){

							if(root[g[from].label][(*it)->elabel].find(g[(*it)->to].label) == root[g[from].label][(*it)->elabel].end()){
								Projected * item = new Projected;
								item->push(g, projDB[i]->tran_id, *it, 0);
								root[g[from].label][(*it)->elabel][g[(*it)->to].label] = item;
							}else{
								root[g[from].label][(*it)->elabel][g[(*it)->to].label]->push (g, projDB[i]->tran_id, *it, 0);
							}
						}
					}
				}
			}

			for (ProjectedP_iterator3 fromlabel = root.begin() ;
				fromlabel != root.end() ; ++fromlabel)
			{
				for (ProjectedP_iterator2 elabel = fromlabel->second.begin() ;
					elabel != fromlabel->second.end() ; ++elabel)
				{
					for (ProjectedP_iterator1 tolabel = elabel->second.begin();
						tolabel != elabel->second.end(); ++tolabel)
					{
						/* Build the initial two-node graph.  It will be grown
						 * recursively within project.
						 */
						Projected * db = tolabel->second;
						db->dfs_code.push(0, 1, fromlabel->first, elabel->first, tolabel->first);
						db->comp_support();
						if(db->is_min()){
							//add to map
							new_PDB[db->dfs_code.back()] = db;
						}else{
							delete db;
						}
					}
				}
			}
		}else{

			/* We just outputted a frequent subgraph.  As it is frequent enough, so
			 * might be its (n+1)-extension-graphs, hence we enumerate them all.
			 */
			const RMPath &rmpath = dfs_code.buildRMPath ();
			int minlabel = dfs_code[0].fromlabel;
			int maxtoc = dfs_code[rmpath[0]].to;

			ProjectedP_map3 new_fwd_root;
			ProjectedP_map2 new_bck_root;
			EdgeList edges;

			/* Enumerate all possible one edge extensions of the current substructure.
			 */
			for (unsigned int n = 0; n < projDB.size(); ++n) {

				unsigned int id = projDB[n]->tran_id;
				PDFS *cur = projDB[n];
				History history (projDB[n]->transaction, cur);

				// XXX: do we have to change something here for directed edges?

				// backward
				for (int i = (int)rmpath.size()-1; i >= 1; --i) {
					Edge *e = get_backward (projDB[n]->transaction, history[rmpath[i]], history[rmpath[0]], history);
					if (e){
						if(new_bck_root[dfs_code[rmpath[i]].from].find(e->elabel) == new_bck_root[dfs_code[rmpath[i]].from].end()){
							Projected * item = new Projected;
							item->push(projDB[n]->transaction, projDB[n]->tran_id, e, cur);
							new_bck_root[dfs_code[rmpath[i]].from][e->elabel] = item;
						}else{
							new_bck_root[dfs_code[rmpath[i]].from][e->elabel]->push(projDB[n]->transaction, projDB[n]->tran_id, e, cur);
						}
					}
				}

				// The problem is:
				// history[rmpath[0]]->to > TRANS[id].size()
				if (get_forward_pure (projDB[n]->transaction, history[rmpath[0]], minlabel, history, edges))
					for (EdgeList::iterator it = edges.begin(); it != edges.end(); ++it){
						if(new_fwd_root[maxtoc][(*it)->elabel].find(projDB[n]->transaction[(*it)->to].label) == new_fwd_root[maxtoc][(*it)->elabel].end()){
							Projected * item = new Projected;
							item->push(projDB[n]->transaction, projDB[n]->tran_id, *it, cur);
							new_fwd_root[maxtoc][(*it)->elabel][projDB[n]->transaction[(*it)->to].label] = item;
						}else{
							new_fwd_root[maxtoc][(*it)->elabel][projDB[n]->transaction[(*it)->to].label]->push(projDB[n]->transaction, projDB[n]->tran_id, *it, cur);
						}
					}

				// backtracked forward
				for (int i = 0; i < (int)rmpath.size(); ++i)
					if (get_forward_rmpath (projDB[n]->transaction, history[rmpath[i]], minlabel, history, edges))
						for (EdgeList::iterator it = edges.begin(); it != edges.end();  ++it){
							if(new_fwd_root[dfs_code[rmpath[i]].from][(*it)->elabel].find(projDB[n]->transaction[(*it)->to].label) == new_fwd_root[dfs_code[rmpath[i]].from][(*it)->elabel].end()){
								Projected * item = new Projected;
								item->push(projDB[n]->transaction, projDB[n]->tran_id, *it, cur);
								new_fwd_root[dfs_code[rmpath[i]].from][(*it)->elabel][projDB[n]->transaction[(*it)->to].label] = item;
							}else{
								new_fwd_root[dfs_code[rmpath[i]].from][(*it)->elabel][projDB[n]->transaction[(*it)->to].label]->push(projDB[n]->transaction, projDB[n]->tran_id, *it, cur);
							}
						}
			}

			/* Test all extended substructures.
			 */
			// backward
			for (ProjectedP_iterator2 to = new_bck_root.begin(); to != new_bck_root.end(); ++to) {
				for (ProjectedP_iterator1 elabel = to->second.begin(); elabel != to->second.end(); ++elabel) {
					Projected * db = elabel->second;
					db->dfs_code.push(maxtoc, to->first, -1, elabel->first, -1);
					db->comp_support();
					if(db->is_min()){
						//add to map
						new_PDB[db->dfs_code.back()] = db;
					}else{
						delete db;
					}
				}
			}

			// forward
			for (ProjectedP_riterator3 from = new_fwd_root.rbegin() ;
				from != new_fwd_root.rend() ; ++from)
			{
				for (ProjectedP_iterator2 elabel = from->second.begin() ;
					elabel != from->second.end() ; ++elabel)
				{
					for (ProjectedP_iterator1 tolabel = elabel->second.begin();
							tolabel != elabel->second.end(); ++tolabel)
					{
						Projected * db = tolabel->second;
						db->dfs_code.push(from->first, maxtoc+1, -1, elabel->first, tolabel->first);
						db->comp_support();
						if(db->is_min()){
							//add to map
							new_PDB[db->dfs_code.back()] = db;
						}else{
							delete db;
						}
					}
				}
			}
		}
	}
};

#endif /* GSPAN_H_ */
