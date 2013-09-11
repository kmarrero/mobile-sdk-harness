#!/bin/sh

if [[ $1 == --merge ]]; then
	#check AppOnMap.cpp not untracked or unstaged in index - if so, fail with error
	git_files_status=`git status -z --porcelain`

	if [[ $git_files_status == *AppOnMap.cpp\?\?* ]] || [[ $git_files_status == *"M AppOnMap.cpp"* ]]; then
		echo "This script will download a new AppOnMap.cpp for the latest platform configuration (and perform a merge). You should either revert, stash, or stage your changes to this file.";
		exit 1
	fi

	if [[ $git_files_status == *AppOnMap.h\?\?* ]] || [[ $git_files_status == *"M AppOnMap.h"* ]]; then
		echo "This script will download a new AppOnMap.h for the latest platform configuration (and perform a merge). You should either revert, stash, or stage your changes to this file.";
		exit 1
	fi
fi

#get latest sdk package
rm -rf ./Include
curl http://s3.amazonaws.com/eegeo-static/sdk.package.tar.gz > ./sdk.package.tar.gz
tar -zxvf ./sdk.package.tar.gz
rm -f ./sdk.package.tar.gz
value=`cat ./sdk.package/version.txt`
echo "Platform version --> $value"
mv ./sdk.package/ ./Include

if [[ $1 == --merge ]]; then
	echo "Merging current version of AppOnMap with canonical version in SDK"

	#merge AppOnMap.cpp/h with the latest from platform using an empty file as ancestor
	touch ./.temp.empty
	mv ./Include/platform/AppOnMap.cpp ./AppOnMap_LATEST.cpp
	git merge-file ./AppOnMap.cpp ./.temp.empty ./AppOnMap_LATEST.cpp
	echo "Counted $? conflicts between current and canonical AppOnMap.cpp"
	rm -f ./AppOnMap_LATEST.cpp
	rm -r ./.temp.empty

	touch ./.temp.empty
	mv ./Include/platform/AppOnMap.h ./AppOnMap_LATEST.h
	git merge-file ./AppOnMap.h ./.temp.empty ./AppOnMap_LATEST.h
	echo "Counted $? conflicts between current and canonical AppOnMap.h"
	rm -f ./AppOnMap_LATEST.h
	rm -r ./.temp.empty
else
	echo "Not merging current version of AppOnMap with canonical version in SDK - note that some API points may have changed. To update with a merge, run this script with --merge"
	rm -f ./Include/platform/AppOnMap.cpp
	rm -f ./Include/platform/AppOnMap.h
fi