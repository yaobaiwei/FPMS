# Frequent Pattern Mining System Project

## User APIs
###1.Pattern&lt;Delta> class
 Users have to define their own pattern class inherit from the **Pattern&lt;Delta> class**, where Delta type is the minimum increment from a pattern to its supper pattern. For example, in FP-growth and Prefixspan algorithms, Delta can be regarded as items in each transactions.  
 Users have to impliment their own **grow(DeltaType& d, Pattern& new_pattern)** function, which will return a new super pattern new_pattern with d appended to the end.
 Users also have to consider how to print the pattern with **print()** function.  
 *UDF:*  
 **grow(DeltaType& d, Pattern& new_pattern)**
 **print()**

###2.Worker&lt;ReadInType,PDB> class
 ReadInType is the type of elements that read in from the files on hdfs. ReadInType data are the also used in initial counting, pruning those globally infrequent elements.  
 There are two user define functions in Worker class. One is **to_record(char* line, RecordContainer& records)**, reponsible for transfering lines read from the file to records of ReadInType elements. Records are vectors of ReadInType elements.   
 The other is **mapping(RecordContainer & records, TransactionContainer  & transactions, unordered_map<ReadInItemT, int> & itemlist)**. This is for slaves to transfer records to transactions, using the globalling frequent elements returned by the master(itemlist). *Records* will be deleted after then, and the following computation depends on *transactions*.  
 *UDF:*  
 **to_record(char* line, RecordContainer& records)**
 **mapping(RecordContainer & records, TransactionContainer  & transactions, unordered_map<ReadInItemT, int> & itemlist)**

###3.ProjTran&lt;TranT, PatternT> class
 Transaction type is the type defined by the users to store projected transactions. The information includes transaction ID, reference of original reference, and other user-define data fields, such as start-position in prefixspan.

###4.PDB&lt;ProjTran> class  
 PDB means projected database. It contains projected transactions of type ProjTran. And we need **init(TransactionContainer & db)** to initialize all projected transactions.
 User have to implement **scan_project()**function for generating new PDB.  
 *UDF:*  
 **init(TransactionContainer & db)**  
 **scan_project()**


##Other System Classes
###SlaveNode
Each SlaveNode contains a pattern. All SlaveNodes are arranged in a tree structure. The children nodes contains patterns that are super patterns of their parent nodes.  
When a new SlaveNode is grown, it will scan the existing projected database and generate all frequent super pattern and their projected database.  

###MasterNode
Each MasterNode contains a pattern, as the SlaveNode. And it also maintain a vector to store the reported support of that pattern from every slaves.  

###Slave  
Slave's job in every round:  
1. Check whether there is message from master. If there is "confirm message" with globally freqent pattern, mark the frequent pattern; if the confirm message is infrequent pattern, delete the SlaveNode and its subtree. If there is "request message", slave try to grow as requested, and report the support back to master.  
2. Locate the right most SlaveNode that can grow new locally frequent pattern, then grow the new SlaveNode.  
3. Try to do garbage collection. Delete those leaf nodes that can no longer grow children.

###Master  
Master does the following job in every round:  
1. Keep waiting for slaves reporting the locally frequent patterns. Add new MasterNode or update the support vector according to the report message. If the pattern is confirmed globally infrequent, delete it and its subtree.  
2. Update the record of the slowest progress of amoung all slaves, called min_frontier.  
3. Scan the tree once according the DFS order, until meet the min_frontier point. Delete all confirmed leaf nodes and send request for those unconfirmed node.

###Note
I worked as a helper of Torby Lee for this system as his FYP.
