#include "CRF.h"

SEXP Decode_Chain(SEXP _crf)
{
	SEXP _nNodes, _nStates, _maxState;
	PROTECT(_nNodes = AS_INTEGER(getListElement(_crf, "n.nodes")));
	PROTECT(_nStates = AS_INTEGER(getListElement(_crf, "n.states")));
	PROTECT(_maxState = AS_INTEGER(getListElement(_crf, "max.state")));
	int nNodes = INTEGER_POINTER(_nNodes)[0];
	int *nStates = INTEGER_POINTER(_nStates);
	int maxState = INTEGER_POINTER(_maxState)[0];

	SEXP _nodePot, _edgePot;
	PROTECT(_nodePot = AS_NUMERIC(getListElement(_crf, "node.pot")));
	PROTECT(_edgePot = AS_NUMERIC(getListElement(_crf, "edge.pot")));
	double *nodePot = NUMERIC_POINTER(_nodePot);
	double *edgePot = NUMERIC_POINTER(_edgePot);

	SEXP _labels;
	PROTECT(_labels = NEW_INTEGER(nNodes));
	int *labels = INTEGER_POINTER(_labels);
	setValues(_labels, labels, -1);

	/* forward pass */

	double *alpha = (double *) R_alloc(nNodes * maxState, sizeof(double));
	for (int i = 0; i < nNodes * maxState; i++)
		alpha[i] = 0;
	double *kappa = (double *) R_alloc(nNodes, sizeof(double));
	for (int i = 0; i < nNodes; i++)
		kappa[i] = 0;
	int *backlinks = (int *) R_alloc(nNodes * maxState, sizeof(int));
	for (int i = 0; i < nNodes * maxState; i++)
		backlinks[i] = 0;

	for (int i = 0; i < nStates[0]; i++)
	{
		alpha[nNodes * i] = nodePot[nNodes * i];
		kappa[0] += alpha[nNodes * i];
	}
	for (int i = 0; i < nStates[0]; i++)
		alpha[nNodes * i] /= kappa[0];

	double *p_alpha, *p0_alpha, *p_nodePot, *p_edgePot;
	double pot, maxPot;
	int maxIndex, *p_backlinks;
	for (int i = 1; i < nNodes; i++)
	{
		p0_alpha = alpha + i;
		p_nodePot = nodePot + i;
		p_edgePot = edgePot + maxState * maxState * (i-1);
		p_backlinks = backlinks + i;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_alpha = alpha + i - 1;
			maxPot = -1;
			maxIndex = -1;
			for (int k=0; k < nStates[i-1]; k++)
			{
				pot = p_alpha[0] * p_edgePot[k];
				if (pot > maxPot)
				{
					maxPot = pot;
					maxIndex = k;
				}
				p_alpha += nNodes;
			}
			p0_alpha[0] = maxPot * p_nodePot[0];
			kappa[i] += p0_alpha[0];
			p_backlinks[0] = maxIndex;
			p0_alpha += nNodes;
			p_nodePot += nNodes;
			p_edgePot += maxState;
			p_backlinks += nNodes;
		}
		p_alpha = alpha + i;
		for (int j = 0; j < nStates[i]; j++)
		{
			p_alpha[0] /= kappa[i];
			p_alpha += nNodes;
		}
	}

	/* backward pass */

	maxPot = -1;
	maxIndex = -1;
	p_alpha = alpha + nNodes - 1;
	for (int i = 0; i < nStates[nNodes-1]; i++)
	{
		if (p_alpha[0] > maxPot)
		{
			maxPot = p_alpha[0];
			maxIndex = i;
		}
		p_alpha += nNodes;
	}
	labels[nNodes-1] = maxIndex;
	
	for (int i = nNodes-1; i > 0; i--)
	{
		labels[i-1] = backlinks[i + nNodes * labels[i]];
	}

	for (int i = 0; i < nNodes; i++)
		labels[i]++;

	UNPROTECT(6);
	return(_labels);
}