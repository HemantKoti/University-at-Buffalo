import re
import sys

[_,graphFileName,treeFileName] = sys.argv


with open(graphFileName,'r') as f:
	match=re.search('([0-9]*) ([0-9]*)',f.readline())
	n=int(match.group(1))
	m=int(match.group(2))
	graph=[{} for i in xrange(n+1)]
	for line in f:
		match=re.search('([0-9]*) ([0-9]*) ([0-9]*)',line)
		u=int(match.group(1))
		v=int(match.group(2))
		c=int(match.group(3))
		graph[u][v]=c
		graph[v][u]=c

with open(treeFileName,'r') as f:
	match=re.search('([0-9]*)',f.readline())
	allegedCost=int(match.group(1))
	tree=[{} for i in xrange(n+1)]
	edgeCount=0
	for line in f:
	        if line.strip() == "":
		        continue
		match=re.search('([0-9]*) ([0-9]*) ([0-9]*)',line)
		u=int(match.group(1))
		v=int(match.group(2))
		c=int(match.group(3))
		tree[u][v]=c
		tree[v][u]=c
		edgeCount+=1
	if edgeCount > n-1:
		print "Graph contains cycle"
		exit()

def dfs(i,visited):
        if not (i in visited):
	        visited[i]=0
	        for child in tree[i]:
		        dfs(child,visited)

visited={}
dfs(1,visited)
for i in xrange(1,n+1):
        if not (i in visited):
	        print "Tree does not span graph"
	        exit()

trueCost=0
for u in xrange(1,n+1):
        for v in tree[u]:
	        if not (v in tree[u]):
		        print "Tree contains an edge not in graph"
		        exit()
	        if u<v:
		        trueCost+=graph[u][v]
if trueCost != allegedCost:
	print "Alleged cost is wrong"
	exit()


print "Tree is spanning with cost", trueCost
