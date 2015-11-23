#!/usr/bin/Rscript

library("methods")
library("flowCore")

tryCatch({
	
	args <- commandArgs(trailingOnly=TRUE)
	fcsfilename <- args[1]

	#extract point data from file 
  	data <- read.FCS(fcsfilename)
  	data <- exprs(data)

	#set values < 1 to 1
  	data[data<1] = 1
	
	#apply log to columns 
	data[, 3:12] = log10(data[, 3:12]) 
 	data <- unique(na.omit(data))

	#normalize columns
	for(j in 1:ncol(data)){
		col_max = max(data[,j])
		if(col_max != 0){
			data[,j] <- data[,j] * (1000 / col_max)
		}
	}

	nrows <- dim(data)[1]
	ncols <- dim(data)[2]
	data <- array(data, dim = c(nrows, ncols))
	print(nrows)

	#create output directory and switch to it
	setwd(sprintf("log_%s_%s", args[1], gsub(" ", "_", date()))
	#pick three random points to be anchor points
	anchors <- data[sample(1:nrows, 3, replace = FALSE), ]

	#write anchor points to file so we know what they are
	write.table(as.matrix(anchors), file="anchorpoints", sep=" ", col.names = FALSE, row.names = FALSE)

	#split the data matrix into submatrices of one row each
	subsets <- split(as.data.frame(data), rep(1:nrows, each = 1))

	#prepend anchor points to each submatrix
	subsets <- lapply(subsets, function(x) { rbind(anchors, x)})

	#compute distance matrices and write them to file
	for(j in 1:length(subsets)){
	
		d <- dist(subsets[[i]], method = "manhattan")
		
		cat(npoints, file=j, sep="\n")	
		write.table(as.matrix(d), file=j, append = TRUE, sep = " ", col.names = FALSE, row.names = FALSE)

	}

	},
	error = function(e) {print(e);}
	)
