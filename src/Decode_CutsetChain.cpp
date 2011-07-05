#include "CRF.h"

SEXP Decode_CutsetChain(SEXP _crf)
{
	CRFclamped crf(_crf);
	crf.Init_Labels();
	crf.Init_NodeBel();
	crf.original.Init_Labels();
	crf.Decode_CutsetChain();
	return(crf.original._labels);
}

void CRFclamped::Decode_CutsetChain()
{
	int *y = (int *) R_alloc(original.nNodes, sizeof(int));
	for (int i = 0; i < original.nNodes; i++)
	{
		if (clamped[i] > 0)
		{
			clamped[i] = 1;
			y[i] = 0;
		}
		else
		{
			clamped[i] = 0;
			y[i] = -1;
		}
	}

	double pot, maxPot = -1;
	int index;
	while (1)
	{
		R_CheckUserInterrupt();

		/* Reset node potentials */
		Reset_NodePot();

		/* Decode clamped CRF */
		Decode_Chain();

		/* Map results back */
		for (int i = 0; i < nNodes; i++)
			y[nodeId[i]-1] = labels[i] - 1;

		/* Calculate potential */
		pot = original.Get_Potential(y);

		/* Record the best potentials */
		if (pot > maxPot)
		{
			maxPot = pot;
			for (int i = 0; i < original.nNodes; i++)
				original.labels[i] = y[i] + 1;
		}

		/* Next configuration */
		for (index = 0; index < original.nNodes; index++)
		{
			if (clamped[index] == 0)
				continue;
			clamped[index]++;
			y[index]++;
			if (y[index] < original.nStates[index])
				break;
			else
			{
				clamped[index] = 1;
				y[index] = 0;
			}
		}

		if (index == original.nNodes)
			break;
	}
}