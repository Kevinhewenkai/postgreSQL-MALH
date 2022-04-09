// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "hash.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
	PageID  curpage;   // current page in scan
	int     is_ovflow; // are we in the overflow pages?
//	Offset  curtup;    // offset of current tuple within page
    char*  curtup;    // offset of current tuple within page
	//TODO
    Offset curTupIndex;    // index for check Is there more tuple in page
    PageID  curScanPage; // overflow page or data page
    Tuple query;
    Bits unknownOffset; // start with 0 while goto next bucket ut plus 1 and | bit in unknown
    Bits checkAllBucket; // if checkAllBucket = 0x00011111111111(num(not 0 unknown)) then we have looped all buckets
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
	printf("starting query\n");
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
    char **attribs = malloc(nattrs(r) * sizeof(char *));
    tupleVals(q, attribs);

    ChVecItem *cv = chvec(r);
    int numberOfUnknownBits = 0;
    // loop each attribute
    for (int i = 0; i < nattrs(r); i ++) {
        // hash = 00101001010101
        // cv= bits,attrib : bits,attrib ...
        // hash == hash of current attrib
        Bits hash = hash_any((unsigned char *)attribs[i], strlen(attribs[i]));
	printf("hash: %d\n", hash);
        // loop each cvItem in choice vector
        for (int j = 0; j < MAXCHVEC; j++) {
            // if cv's attrib = the attrib we are scanning,
            if (cv[j].att == i) {
                if (strcmp(attribs[i], "?") != 0) {
                    // set known bits at position cv.bits where the given query attrib is not ?
                    // get bits == cv.pos
                    if (bitIsSet(hash, j)) {
                        new->known = setBit(new->known, j);
                    } else {
                        new->known = unsetBit(new->known, j);
                    }
               } else {
                    new->unknown = setBit(new->unknown, j);
                    numberOfUnknownBits++;
               }
           }
        }
    }
    // form unknown bits from '?' attributes

    // TODO lecture linear hashing 4
    printf("known: %d\n", new->known);
    printf("unknow: %d\n", new->unknown);

    // TODO maybe because of depth
    PageID pid = getLower(new->known, depth(r));
    if (pid < splitp(r)) {
        pid = getLower(new->known, depth(r)+1);
    }

    new->rel = r;
    new->curpage = pid;
    new->is_ovflow = 0;
    printf("1111111111\n");
    Page page = getPage(dataFile(r), pid);
    new->curtup = pageData(page);
    new->curTupIndex = 0;
    new->curScanPage = pid;
    new->query = q;
    new->unknownOffset = 0;

    for (int i = 0; i < numberOfUnknownBits; i++) {
        new->checkAllBucket = new->checkAllBucket << 1;
        new->checkAllBucket = new->checkAllBucket | 1;
    }
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object
	return new;
}

//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
int gotoNextPage(Query q) {
    Bits nextBucket = q->known;
    if (q->unknownOffset == q->checkAllBucket) {
        return 1;
    }
    q->unknownOffset += 1;
    for (int i = 0; i < MAXBITS; i++) {
        if (bitIsSet(q->unknown, i)) {
            nextBucket = nextBucket | (q->unknownOffset & 1);
        }
    }
    q->curScanPage = nextBucket;
    q->curpage = nextBucket;
    q->curtup = pageData(getPage(dataFile(q->rel), q->curScanPage)) + 1;
    return 0;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
    // if (more tuples in current page)
    //    get next matching tuple from current page
    while (1) {
        Page page = getPage(dataFile(q->rel), q->curScanPage);
        printf(pageData(page));
        Tuple tuple;
        if (q->curTupIndex <= pageNTuples(page)) {
            // jump to the next tuple
            tuple = q->curtup;
            // TODO tuple = 0
            if (tupleMatch(q->rel, tuple, q->query)) {
                // move to the next tuple
                q->curtup = q->curtup + strlen(q->curtup) + 1;
                return tuple;
            }
            q->curTupIndex++;
        }
            // else if (current page has overflow)
            //    move to overflow page
            //    grab first matching tuple from page
        else if (pageOvflow(page) != NO_PAGE) {
            q->curScanPage = pageOvflow(page);
            q->curTupIndex = 0;
            q->curtup = pageData(getPage(dataFile(q->rel), pageOvflow(page)));
            continue;
        }
        // else
        //    move to "next" bucket
        //    grab first matching tuple from data page
        // endif
        //     E.g. assuming only 8 bits, known bits = 00110001, unknown bits = 10000100
        //     We need to fill two bits to make a complete bit-string; assume those two bits are 01
        //     The complete pattern for the unknown bits is 00000100
        //     Overlaying this on 00110001 gives 00110101 which is binary for 53 (I hope)
        //     So you access page 53
        //     There are three other bit patterns to fill the unknown bits 11, 10, 00 (as well as 01)
        else {
            if (gotoNextPage(q)) return NULL;
            continue;
        }
        // if (current page has no matching tuples)
        //    go to next page (try again)
        // endif

        if (gotoNextPage(q)) return NULL;
    }
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
    free(q);
}
