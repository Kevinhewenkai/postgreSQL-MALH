// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
	PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
	Offset  curtup;    // offset of current tuple within page
	//TODO
    Offset curAttrib;    // which attribute in the tuple we are looking at
    PageID  curScanPage; // overflow page or data page
    char *query;
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
	// Partial algorithm
    // attrib[i] == 1 means the ith attribute of r is known else = 0 (for ?)
    int attrib[nattrs(r)];
    int i = 0;
    while (*q != '\0') {
        char *j = q + 1;
        char *k = q - 1;
        if ((*j == ',' || *j == '\0') && *k == ',' && *q =='?') {
            attrib[i] = 0;
            i++;
        } else if ((*j == ',' || *j == '\0') && *q !='?'){
            attrib[i] = 1;
            i++;
        }
        q++;
    }

    // form the know and unknown attributes
    ChVecItem *cv = chvec(r);
    for (int j = 0; j < depth(r); j++) {
        if(attrib[cv[j].att]) {
            // form known bits from known attributes
            setBit(new->known, j);
            // form unknown bits from '?' attributes
            unsetBit(new->unknown, j);
        } else{
            // form unknown bits from '?' attributes
            unsetBit(new->known, j);
            // form known bits from known attributes
            setBit(new->unknown, j);
        }
    }

    PageID pid = new->known;
    new->rel = r;
    new->curpage = pid;
    new->is_ovflow = 0;
    new->curtup = 0;
    new->curAttrib = 0;
    new->curScanPage = pid;
    new->query = q;
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object
	return new;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	// if (more tuples in current page)
	//    get next matching tuple from current page
	// else if (current page has overflow)
	//    move to overflow page
	//    grab first matching tuple from page
	// else
	//    move to "next" bucket
	//    grab first matching tuple from data page
	// endif
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	return NULL;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
    free(q);
}
