ifndef GIT_MK_INCLUDED
GIT_MK_INCLUDED = 1

.PHONY: git-submodules

git-submodules:
	git submodule update --init --recursive --depth 1

endif
