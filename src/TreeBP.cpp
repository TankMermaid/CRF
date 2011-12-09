#include "CRF.h"

/* Tree BP */

void CRF::TreeBP(double *messages_1, double *messages_2, bool maximize)
{
	for (int i = 0; i < maxState * nEdges; i++)
		messages_1[i] = messages_2[i] = 1;

	int *nWaiting = (int *) R_alloc(nNodes, sizeof(int));
	int **waiting = (int **) R_alloc(nNodes, sizeof(int *));
	int *sent = (int *) R_alloc(nNodes, sizeof(int));
	int senderHead, senderTail, nReceiver;
	int *sender = (int *) R_alloc(nNodes * 2, sizeof(int));
	int *receiver = (int *) R_alloc(nNodes, sizeof(int));
	double **incoming = (double **) R_alloc(nNodes, sizeof(double *));

	senderHead = senderTail = nReceiver = 0;
	for (int i = 0; i < nNodes; i++)
	{
		nWaiting[i] = nAdj[i];
		waiting[i] = (int *) R_alloc(nAdj[i], sizeof(int));
		for (int j = 0; j < nAdj[i]; j++)
			waiting[i][j] = 1;
		sent[i] = -1;
		if (nAdj[i] == 1)
			sender[senderTail++] = i;
		incoming[i] = (double *) R_alloc(maxState, sizeof(double));
		for (int j = 0; j < nStates[i]; j++)
			incoming[i][j] = NodePot(i, j);
	}

	int s, r, e, n;
	double mesg, sumMesg, *p_messages;
	double *outgoing = (double *) R_alloc(maxState, sizeof(double));

	while (senderHead < senderTail)
	{
		R_CheckUserInterrupt();

		s = sender[senderHead++];
		if (sent[s] == -2) continue;

		nReceiver = 0;
		if (nWaiting[s] == 1)
		{
			for (int i = 0; i < nAdj[s]; i++)
			{
				if (waiting[s][i])
				{
					receiver[nReceiver++] = i;
					sent[s] = nAdj[s] == 1 ? -2 : i;
					break;
				}
			}
		}
		else
		{
			for (int i = 0; i < nAdj[s]; i++)
				if (sent[s] != i)
					receiver[nReceiver++] = i;
			sent[s] = -2;
		}

		for (int i = 0; i < nReceiver; i++)
		{
			n = receiver[i];
			r = AdjNodes(s, n);

			for (int j = 0; j < nAdj[r]; j++)
				if (AdjNodes(r, j) == s)
				{
					waiting[r][j] = 0;
					nWaiting[r]--;
					break;
				}

			if (sent[r] != -2 && nWaiting[r] <= 1)
				sender[senderTail++] = r;

			/* send messages */

			e = AdjEdges(s, n);
			sumMesg = 0;
			if (EdgesBegin(e) == s)
			{
				p_messages = messages_1 + maxState * e;
				for (int j = 0; j < nStates[s]; j++)
					outgoing[j] = p_messages[j] == 0 ? 0 : incoming[s][j] / p_messages[j];
				p_messages = messages_2 + maxState * e;
				for (int j = 0; j < nStates[r]; j++)
				{
					p_messages[j] = 0;
					if (maximize)
					{
						for (int k = 0; k < nStates[s]; k++)
						{
							mesg = outgoing[k] * EdgePot(e, k, j);
							if (mesg > p_messages[j])
								p_messages[j] = mesg;
						}
					}
					else
					{
						for (int k = 0; k < nStates[s]; k++)
							p_messages[j] += outgoing[k] * EdgePot(e, k, j);
					}
					sumMesg += p_messages[j];
				}
			}
			else
			{
				p_messages = messages_2 + maxState * e;
				for (int j = 0; j < nStates[s]; j++)
					outgoing[j] = p_messages[j] == 0 ? 0 : incoming[s][j] / p_messages[j];
				p_messages = messages_1 + maxState * e;
				for (int j = 0; j < nStates[r]; j++)
				{
					p_messages[j] = 0;
					if (maximize)
					{
						for (int k = 0; k < nStates[s]; k++)
						{
							mesg = outgoing[k] * EdgePot(e, j, k);
							if (mesg > p_messages[j])
								p_messages[j] = mesg;
						}
					}
					else
					{
						for (int k = 0; k < nStates[s]; k++)
						{
							p_messages[j] += outgoing[k] * EdgePot(e, j, k);
						}
					}
					sumMesg += p_messages[j];
				}
			}
			for (int j = 0; j < nStates[r]; j++)
			{
				p_messages[j] /= sumMesg;
				incoming[r][j] *= p_messages[j];
			}
		}
	}
}
