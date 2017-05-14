#!/bin/sh
#
# Bumps the micro according to the number of commits
#
# To have git run this script on commit, create a "pre-commit" text file in
# .git/hooks/ with the following content:
# #!/bin/sh
# if [ -x ./_pre-commit.sh ]; then
# 	source ./_pre-commit.sh
# fi

type -P sed &>/dev/null || { echo "sed command not found. Aborting." >&2; exit 1; }
type -P git &>/dev/null || { echo "git command not found. Aborting." >&2; exit 1; }

MICRO=`git rev-list HEAD --count`
((MICRO++))
if [ -f ./.amend ]; then
	((MICRO--))
	rm ./.amend;
fi
echo "setting micro to $MICRO"

cat > cmd.sed <<\_EOF
s/^[ \t]*FILEVERSION[ \t]*\([0-9]*\),\([0-9]*\),[0-9]*,\(.*\)/ FILEVERSION \1,\2,@@MICRO@@,\3/
s/^[ \t]*PRODUCTVERSION[ \t]*\([0-9]*\),\([0-9]*\),[0-9]*,\(.*\)/ PRODUCTVERSION \1,\2,@@MICRO@@,\3/
s/^\([ \t]*\)VALUE[ \t]*"FileVersion",[ \t]*"\(.*\)\..*"/\1VALUE "FileVersion", "\2.@@MICRO@@"/
s/^\([ \t]*\)VALUE[ \t]*"ProductVersion",[ \t]*"\(.*\)\..*"/\1VALUE "ProductVersion", "\2.@@MICRO@@"/
s/^#define VERSION\(.*\)"\(.*\)\..*".*/#define VERSION\1"\2.@@MICRO@@"/
s/^PSP_EBOOT_TITLE\(.*\)v\([0-9]*\).\([0-9]*\).[0-9]*/PSP_EBOOT_TITLE\1v\2.\3.@@MICRO@@/
_EOF

# First run sed to substitute our variable in the sed command file
sed -i -e "s/@@MICRO@@/$MICRO/g" cmd.sed

# Run sed to update the nano version, and add the modified files
sed -b -i -f cmd.sed colditz.rc
sed -i -f cmd.sed colditz.h
sed -i -f cmd.sed Makefile
git add colditz.rc colditz.h Makefile
rm cmd.sed
