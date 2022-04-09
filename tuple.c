// tuple.c ... functions on tuples
// part of Multi-attribute Linear-hashed Files
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "tuple.h"
#include "reln.h"
#include "hash.h"
#include "chvec.h"
#include "bits.h"

// return number of bytes/chars in a tuple

int tupLength(Tuple t)
{
	return strlen(t);
}

// reads/parses next tuple in input

Tuple readTuple(Reln r, FILE *in)
{
	char line[MAXTUPLEN];
	if (fgets(line, MAXTUPLEN-1, in) == NULL)
		return NULL;
	line[strlen(line)-1] = '\0';
	// count fields
	// cheap'n'nasty parsing
	char *c; int nf = 1;
	for (c = line; *c != '\0'; c++)
		if (*c == ',') nf++;
	// invalid tuple
	if (nf != nattrs(r)) return NULL;
	return copyString(line); // needs to be free'd sometime
}

// extract values into an array of strings

void tupleVals(Tuple t, char **vals)
{
	char *c = t, *c0 = t;
	int i = 0;
	for (;;) {
		while (*c != ',' && *c != '\0') c++;
		if (*c == '\0') {
			// end of tuple; add last field to vals
			vals[i++] = copyString(c0);
			break;
		}
		else {
			// end of next field; add to vals
			*c = '\0';
			vals[i++] = copyString(c0);
			*c = ',';
			c++; c0 = c;
		}
	}
}

// release memory used for separate attirubte values

void freeVals(char **vals, int nattrs)
{
	int i;
	for (i = 0; i < nattrs; i++) free(vals[i]);
}

// hash a tuple using the choice vector
// lecture muti-dimensional Hashing page 7
// TODO: actually use the choice vector to make the hash

Bits tupleHash(Reln r, Tuple t)
{
	printf("tupleHash");
	char buf[MAXBITS+1];
    // nvals == numbers of attributes
	Count nvals = nattrs(r);
    // vals == a list of string of each attribute
	char **vals = malloc(nvals*sizeof(char *));
	assert(vals != NULL);
    // extract the attribute from tuple t to vals
	tupleVals(t, vals);
    //	Bits hash = hash_any((unsigned char *)vals[0],strlen(vals[0]));
    // start
    Bits hash = 0, oneBit;
    // create a bit string list to store bit string for each attribute after hashed
    Bits attribHashedBitsList[nvals + 1];
    for (int i = 0; i < nvals; i++) {
        attribHashedBitsList[i] = hash_any((unsigned  char *) vals[i], strlen(vals[i]));
    }

    ChVecItem *c = chvec(r);
    // loop each hashed bit to get the result
    for (int i = 0; i < MAXCHVEC; i++) {
        // cv = (attr 1, bit 1)1(attr 2, bit 0)
        Byte cvAttrib = c[i].att;
        Byte cvBit = c[i].bit;
        oneBit = bitIsSet(attribHashedBitsList[cvAttrib], cvBit);
        hash = hash | (oneBit << i);
    }
    // end

    // from bits 0x... (hash) to "..." (buf)
	bitsString(hash,buf);
	printf("hash(%s) = %s\n", t, buf);
    free(vals);
	return hash;
}

// compare two tuples (allowing for "unknown" values)

Bool tupleMatch(Reln r, Tuple t1, Tuple t2)
{
	Count na = nattrs(r);
	char **v1 = malloc(na*sizeof(char *));
	tupleVals(t1, v1);
	char **v2 = malloc(na*sizeof(char *));
	tupleVals(t2, v2);
	Bool match = TRUE;
	int i;
	for (i = 0; i < na; i++) {
		// assumes no real attribute values start with '?'
		if (v1[i][0] == '?' || v2[i][0] == '?') continue;
		if (strcmp(v1[i],v2[i]) == 0) continue;
		match = FALSE;
	}
	freeVals(v1,na); freeVals(v2,na);
	return match;
}

// puts printable version of tuple in user-supplied buffer

void tupleString(Tuple t, char *buf)
{
	strcpy(buf,t);
}
