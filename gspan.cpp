/*
 * gspan.cpp
 *
 *  Created on: 2016年6月30日
 *      Author: yaobaiwei
 */
#include "system/prefixspan/Worker.h"
#include "gspan.h"
#include <stdlib.h>

class gSpan : public Worker<LabelEdge,  Projected >
{
private:
	bool directed;
	bool load_end;
	vector<Graph> graphs;

public:

	gSpan(bool _directed){
		directed = _directed;
		load_end= false;
	}

	virtual void to_record(char* line, RecordContainer & records)
	{
		vector <string> result;
		tokenize<string>(line, back_inserter (result));

		if(!load_end && !result.empty()){
			if(result[0] == "t" && result.size() >= 3){
				int num = atoi(result[2].c_str());
				if(num < 0){
					load_end = true;
					return;
				}
				Graph empty;
				graphs.push_back(empty);
				Record r;
				records.push_back(r);
			}else if (result[0] == "v" && result.size() >= 3) {
				Graph & g = graphs.back();
				unsigned int id    = atoi (result[1].c_str());
				if((id+1) > g.size())
					g.resize (id + 1);
				g[id].label = atoi (result[2].c_str());
			} else if (result[0] == "e" && result.size() >= 4) {
				Graph & g = graphs.back();
				Record & r = records.back();

				int from   = atoi (result[1].c_str());
				int to     = atoi (result[2].c_str());
				int elabel = atoi (result[3].c_str());

				if (g.size () <= from || g.size () <= to) {
					std::cerr << "Format Error:  define vertex lists before edges" << std::endl;
					exit (-1);
				}

				ReadInItemT tmp = ReadInItemT(g[from].label, g[to].label, elabel);
        		if(tmp.fromL > tmp.toL) swap(tmp.fromL, tmp.toL);
        		r.push_back(tmp);

				g[from].push (from, to, elabel);
				if (directed == false)
					g[to].push (to, from, elabel);
			}
		}
	}

	/*
	 * record -> Tran
	 * remove the infrequent edges in each record
	 * invoke the buildEdge(), [gspan code]
	 */
    void rec_to_trans(Graph & g, Transaction & t,  unordered_map<LabelEdge,int> & freqEdge)
    {
    	unordered_map<LabelEdge, int>::iterator sIt;
    	for(int i=0; i<g.size(); i++)
    	{
    		TItemT & item = g[i];
    		TItemT v;
    		v.label = item.label;
    		int edgeNum = item.edge.size();
    		for(int j = 0 ; j < edgeNum; j++){
    			LabelEdge tmp = LabelEdge(g[item.edge[j].from].label, g[item.edge[j].to].label, item.edge[j].elabel);
    			sIt = freqEdge.find(tmp);
    			if(sIt != freqEdge.end()){
    				v.edge.push_back(item.edge[j]);
    			}
    		}
    		t.push_back(v);
    	}
    	t.buildEdge(); //gspan code in Yan, 2002
    }

    virtual void mapping(RecordContainer & records, TransactionContainer  & transactions,  unordered_map<LabelEdge,int> & itemlist)
    {
    	for(int i = 0 ; i < records.size(); i++){
            Transaction t;
    		rec_to_trans(graphs[i], t, itemlist);
    		records[i].clear();
    		graphs[i].clear();
    		if(t.size() > 0){
                transactions.push_back(t);
    		}
    	}
    }
};

int main(int argc, char* argv[]){
	WorkerParams params;
	params.input_path = "/gspan";
	params.threshold = 0.2;
	bool directed = false;
	gSpan worker(directed);
	worker.run(params);
	return 0;
}
