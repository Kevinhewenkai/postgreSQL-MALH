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
	Offset  curtup;    // offset of current tuple within page
//    char*  curtup;    // offset of current tuple within page
	//TODO
    Offset curTupIndex;    // index for check Is there more tuple in page
    PageID  curScanPage; // overflow page or data page
    Tuple query;
    Bits offsetNeedPlusDepth;
    int depth;
    Bits unknownOffset; // start with 0 while goto next bucket ut plus 1 and | bit in unknown
    Bits checkAllBucket; // if checkAllBucket = 00011111111111(num(not 0 unknown)) then we have looped all buckets
    int depth1;
};

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q)
{
//    char *chair = "chair";
//    char *shoes = "shoes";
//    char buf[MAXBITS + 1];
//    Bits testHash = hash_any((unsigned char *)chair, strlen(chair));
//    bitsString(testHash, buf);
//    printf("hash of chair %s\n", buf);
//    testHash = hash_any((unsigned char *)shoes, strlen(shoes));
//    bitsString(testHash, buf);
//    printf("hash of shoes %s\n", buf);
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
//        printf("hash %d\n", hash);
        // loop each cvItem in choice vector
        for (int j = 0; j < MAXCHVEC; j++) {
            // if cv's attrib = the attrib we are scanning,
            if (cv[j].att == i) {
//                printf("attr[i] %s, i = %d\n", attribs[i], i);
                if (strcmp(attribs[i], "?") != 0) {
                    // set known bits at position cv.bits where the given query attrib is not ?
                    // get bits == cv.pos
//                    printf("attr: %u bits %u attrib %s\n", cv[j].att, cv[j].bit, attribs[i]);
//                    printf("bit is set: %d\n", bitIsSet(hash, cv[j].bit));
//                    bitsString(new->known, buf);
//                    printf("known: %s\n", buf);
//                    bitsString(new->unknown,buf);
//                    printf("unknown: %s\n", buf);
                    if (bitIsSet(hash, cv[j].bit)) {
                        new->known = setBit(new->known, j);
                    } else {
                        new->known = unsetBit(new->known, j);
                    }
               } else {
                    new->unknown = setBit(new->unknown, j);
                    if (j < depth(r)) {
                        numberOfUnknownBits++;
                    }
                    if (j == depth(r)) {
                        new->depth1 = 1;
                        numberOfUnknownBits++;
                    }
               }
           }
        }
    }
    // form unknown bits from '?' attributes
//    char buf[MAXCHVEC+1];
//    bitsString(new->known, buf);
//    printf("known: %s\n\n", buf);
//    bitsString(new->unknown, buf);
//    printf("unknown: %s\n\n", buf);
//    printf("depth: %d\n\n", depth(r));
    // TODO lecture linear hashing 4s
    PageID pid = getLower(new->known, depth(r));
//    if (pid < splitp(r)) {
//        pid = getLower(new->known, depth(r)+1);
//    }

    new->rel = r;
    // printf("SETUP first page is %d\n", pid);
    new->curpage = pid;
    new->is_ovflow = 0;
//    printf("Line80\n\n");
//    printf("line 81\n\n");
    new->curtup = 0;
    new->curTupIndex = 0;
    new->curScanPage = pid;
    new->query = q;
    new->unknownOffset = 0;
    new->depth = depth(r);
    if (new->depth1) {
        new->depth++;
    }

    for (int i = 0; i < numberOfUnknownBits; i++) {
//        new->checkAllBucket++;
        new->checkAllBucket = new->checkAllBucket | (1 << i);
    }

//    char buf[MAXCHVEC+1];
//    bitsString(new->checkAllBucket, buf);
//    printf("check: %s\n\n", buf);

    Bits tmp = new->checkAllBucket;
    new->offsetNeedPlusDepth = new->known;
    for (int i = 0; i < new->depth; i++) {
        if (bitIsSet(new->unknown, i)) {
            new->offsetNeedPlusDepth = new->offsetNeedPlusDepth | ((tmp & 1) << i);
            tmp = tmp >> 1;
        }
    }
    new->offsetNeedPlusDepth = getLower(new->offsetNeedPlusDepth, depth(r));
//    char buf[MAXCHVEC+1];
//    bitsString(new->offsetNeedPlusDepth, buf);
//    printf("offset need to plus: %s\n\n", buf);


	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object
	return new;
}

int gotoNextPage(Query q) {
    Bits nextBucket = q->known;
//    printf("checkBucket: %d Offset: %d\n\n", q->checkAllBucket, q->unknownOffset);
    if (q->unknownOffset > q->checkAllBucket) {
        // printf("return 1\n\n");
        return 1;
    }
    q->unknownOffset += 1;
    Bits tmp = q->unknownOffset;
//    printf("offset: %d\n\n", q->unknownOffset);
//    printf("known: %d\n\n", q->known);
//    printf("unknown: %d\n\n", q->unknown);
    for (int i = 0; i < MAXBITS; i++) {
        if (bitIsSet(q->unknown, i)) {
            nextBucket = nextBucket | ((tmp & 1) << i);
            tmp = tmp >> 1;
        }
    }
//    Bits tmpBucket = nextBucket;
    nextBucket = getLower(nextBucket, q->depth);
//    printf("sp: %d\n", splitp(q->rel));
//    printf("q->curpage : %d\n", q->curpage);
    if (q->depth == depth(q->rel) && q->curpage >= q->offsetNeedPlusDepth) {
//        printf("1111\n");
        q->depth++;
        q->unknownOffset = 0;
        if (q->curpage < getLower(q->known, q->depth)) {
            nextBucket = getLower(q->known, q->depth);
        }
//        printf("if statement depth = %d\n", d);
//        char buf[MAXBITS+1];
//        bitsString(nextBucket, buf);
//        printf("buf : %s\n", buf);
//        bitsString(q->unknownOffset, buf);
//        printf("offset: %s\n", buf);
//        nextBucket = getLower(tmpBucket, q->depth);
    }

//     printf("next bucket: %d\n\n", nextBucket);
    // else will happen goto page 1,3,5,7, 7, 9, 18, 1, 3, 5, 7....
    if (nextBucket <= q->curpage) return 1;
    if (nextBucket >= npages(q->rel)) return 1;
    q->curpage = nextBucket;
    q->curScanPage = nextBucket;
//    printf("line 113\n\n");
    q->curtup = 0;
    q->is_ovflow = 0;
    q->curTupIndex = 0;
    return 0;
}

// get next tuple during a scan

Tuple getNextTuple(Query q)
{
	// Partial algorithm:
    // if (more tuples in current page)
    //    get next matching tuple from current page
//    printf("start looping\n");
    while (1) {
        FILE *file = (q->is_ovflow) ? ovflowFile(q->rel) : dataFile(q->rel);
//        printf("curPage: %d\n\n", q->curScanPage);
//        printf("Is overflow: %d\n\n", q->is_ovflow);
    //    printf("curTuple index: %d\n\n", q->curTupIndex);
        Page page = getPage(file, q->curScanPage);
//        printf("page have n tuple: %d\n\n", pageNTuples(page));
//        printf("tuple: %s\n\n", tuple);
        if (q->curTupIndex < pageNTuples(page)) {
            while (q->curTupIndex < pageNTuples(page)) {
                char *tuple = pageData(page);
                // jump to the next tuple
                tuple += q->curtup;
//                printf("tuple: %s\n", tuple);
                q->curTupIndex++;
                if (tupleMatch(q->rel, tuple, q->query)) {
                    q->curtup += tupLength(tuple) + 1;
                    // move to the next tuple
//                    printf("success, tuple: %s\n", tuple);
                    return tuple;
                }
                q->curtup += tupLength(tuple) + 1;
//                free(tuple);
            }
//            continue;
        }
            // else if (current page has overflow)
            //    move to overflow page
            //    grab first matching tuple from page
        if (pageOvflow(page) != NO_PAGE) {
//            printf("overflow!!\n\n");
            q->curScanPage = pageOvflow(page);
//            printf("NEXT OVERFLOW %d\n\n", q->curScanPage);
            q->curTupIndex = 0;
            q->is_ovflow = 1;
            q->curtup = 0;
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
            int check = gotoNextPage(q);
            if (check) {
                // printf("check return NULL");
                return NULL;
            }
//            printf("nextBucket!! Bucket number = %d\n\n", q->curpage);
//            printf("npages %d\n", npages(q->rel));
//            char buf[MAXCHVEC+1];
//            bitsString(q->known, buf);
//            printf("known: %s\n", buf);
//            bitsString(q->unknown, buf);
//            printf("unknown: %s\n", buf);
//            bitsString(q->curpage, buf);
//            printf("curPage %s\n", buf);
//            bitsString(q->unknownOffset, buf);
//            printf("unknownOffset %s\n", buf);
//            printf("depth: %d\n", q->depth);
//            printf("\n");
            //        printf("check %d\n\n", check);
        }
    // if (current page has no matching tuples)
    //    go to next page (try again)
    // endif
    }
//    printf("111111111\n");
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
    free(q);
}
