#!/bin/bash

# Set pre-rawhide branch
PRE="15"

usage(){
    echo "Usage: re [--ver <version> ] [--repo <repo>] [--branch <branch>]"
    echo "  where"
    echo "      version = 6"
    echo "      repo = free | nonfree | fixes"
    echo "      branch = releases | updates | updates-testing"
    exit 1;
}

reindex_releases(){
cd $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/

find . -type f -name "*.rpm" -exec chmod 644 {} \;
find . -type f -name "*.rpm" -exec chown 0.0 {} \;

rm -v $(repomanage -o source/SRPMS/) 2> /dev/null
createrepo -d --update source/SRPMS/

for arch in $ARCH; do
  rm -v $(repomanage -o $arch/debug/) 2> /dev/null
  createrepo -d --update $arch/debug/

  # First make deltas then remove old rpms then reindex again
#  /usr/sbin/chroot /root/fedora-chroot createrepo -x *debuginfo* --deltas \
#      --oldpackagedirs $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/$arch/os/ -d --update \
#      -g /root/comps/$VERSION/reremix-$REPO.xml $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/$arch/os/

  rm -v $(repomanage -o $arch/os/) 2> /dev/null

  createrepo -x *debuginfo* -d --update -g /root/comps/$VERSION/reremix-$REPO.xml \
      $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/$arch/os/
done
}

reindex_updates(){
cd $RFDIR/$REPO/el/$BRANCH/$VERSION/

find . -type f -name "*.rpm" -exec chmod 644 {} \;
find . -type f -name "*.rpm" -exec chown 0.0 {} \;

# source
rm -v $(repomanage -o SRPMS/) 2> /dev/null
createrepo -d --update SRPMS/

for arch in $ARCH; do
  # debug
  rm -v $(repomanage -o $arch/debug/) 2> /dev/null
  createrepo -d --update $arch/debug/

  # arch
  if [ "$REPO" == "fixes" ]; then
    OLDPACKDIR=" --oldpackagedirs $UPDATESDIR/${VERSION}x/$arch/updates/security/ --oldpackagedirs $RELEASEDIR/${VERSION}x/$arch/os/Packages/"
  fi

#  /usr/sbin/chroot /root/fedora-chroot createrepo -d -g /root/comps/$VERSION/reremix-$REPO.xml \
#      -x *debuginfo* --deltas $OLDPACKDIR --oldpackagedirs=$RFDIR/$REPO/el/updates/$VERSION/$arch/ \
#      --oldpackagedirs=$RFDIR/$REPO/el/releases/$VERSION/Everything/$arch/os/ \
#      $RFDIR/$REPO/el/updates/$VERSION/$arch/

  rm -v $(repomanage -o $arch) 2> /dev/null

  /usr/sbin/chroot /root/fedora-chroot createrepo -d -g /root/comps/$VERSION/reremix-$REPO.xml \
      -x *debuginfo* $RFDIR/$REPO/el/updates/$VERSION/$arch/
done
}

reindex_updates-testing(){
cd $RFDIR/$REPO/el/updates/testing/$VERSION/

find . -type f -name "*.rpm" -exec chmod 644 {} \;
find . -type f -name "*.rpm" -exec chown 0.0 {} \;

# source
rm -v $(repomanage -o SRPMS/) 2> /dev/null
createrepo -d --update SRPMS/

for arch in $ARCH; do
  rm -v $(repomanage -o $arch/debug/) 2> /dev/null
  createrepo -d --update $arch/debug/

  if [ "$REPO" == "fixes" ]; then
    OLDPACKDIR=" --oldpackagedirs $UPDATESDIR/${VERSION}x/$arch/updates/security/ --oldpackagedirs $RELEASEDIR/${VERSION}x/$arch/os/Packages/ --oldpackagedirs $UPDATESDIR/${VERSION}x/$arch/updates/fastbugs/"
  fi

#  /usr/sbin/chroot /root/fedora-chroot createrepo -d -g /root/comps/$VERSION/reremix-$REPO.xml \
#      -x *debuginfo* --deltas $OLDPACKDIR --oldpackagedirs $RFDIR/$REPO/el/updates/$VERSION/$arch/ \
#      --oldpackagedirs $RFDIR/$REPO/el/updates/testing/$VERSION/$arch/ \
#      --oldpackagedirs $RFDIR/$REPO/el/releases/$VERSION/Everything/$arch/os/ $RFDIR/$REPO/el/updates/testing/$VERSION/$arch/

  rm -v $(repomanage -o $arch) 2> /dev/null

  /usr/sbin/chroot /root/fedora-chroot createrepo -d -g /root/comps/$VERSION/reremix-$REPO.xml \
      -x *debuginfo* $RFDIR/$REPO/el/updates/testing/$VERSION/$arch/
done
}

if [ "$(id -u)" != "0" ]; then
	echo "Run progrum under superuser. Abort..."
	exit 1;
fi

ARCH="i386 x86_64"
RFDIR="/mirror/fedora/russianfedora/russianfedora"
UPDATESDIR="/mirror/scientificlinux/"
UPTESTDIR="/mirror/scientificlinux/"
RELEASEDIR="/mirror/scientificlinux/"
SOURCEDIR="/home/ashejn/el"
REINDEX="false"

while [ $# -gt 0 ]; do
  case $1 in
    --ver)
      VERSION=$2
      shift; shift
      ;;
    --repo)
      REPO=$2
      shift; shift
      ;;
    --branch)
      BRANCH=$2
      shift; shift
      ;;
    --help)
      usage
      ;;
    *)
      usage
     ;;
  esac
done

if [ "$REPO" == "" -a "$VERSION" == "" -a "$BRANCH" == "" ]; then
  DIRECTORIES=$(find $SOURCEDIR -type f -name "*.rpm" | cut -d "/" -f -7 | uniq)

  for i in $DIRECTORIES; do
    VERSION=$(echo $i | awk -F/ '{ print $5}')
    REPO=$(echo $i | awk -F/ '{ print $6}')

    if [ "$VERSION" == "development" ]; then
      BRANCH="development/rawhide"
    elif [ "$VERSION" == "pre" ]; then
      BRANCH="development/$PRE"
    else
      BRANCH=$(echo $i | awk -F/ '{ print $7}')
    fi

    case $BRANCH in
      releases)
        unset LINK32PACKS
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.src.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/source/SRPMS/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.x86_64.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/x86_64/debug
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.x86_64.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/x86_64/os/

	for noarch in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.noarch.rpm); do
  	  mv -v $noarch $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/i386/os/
	  cd $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/x86_64/os/
	  ln -vf ../../i386/os/$(basename $noarch)
	  cd -
	done

        for lib in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm | grep -v "debuginfo" | grep -v "java-1.6.0-sun"); do
          $(rpm -qlp $lib 2>/dev/null | grep "/usr/lib/" | grep -v "/usr/lib/python" > /dev/null) && LINK32PACKS="$LINK32PACKS $(basename $lib)"
        done

	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.i?86.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/i386/debug/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/i386/os/

        cd $RFDIR/$REPO/el/$BRANCH/$VERSION/Everything/x86_64/os/
        for pack in $LINK32PACKS; do
          ln -vf ../../i386/$pack
        done
        cd -

	reindex_releases
	REINDEX="true"
	;;
      updates)
        unset LINK32PACKS
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.src.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/SRPMS/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.x86_64.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/x86_64/debug
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.x86_64.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/x86_64/

	for noarch in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.noarch.rpm); do
	  mv -v $noarch $RFDIR/$REPO/el/$BRANCH/$VERSION/i386/
	  cd $RFDIR/$REPO/el/$BRANCH/$VERSION/x86_64/
	  ln -vf ../i386/$(basename $noarch)
	  cd -
	done

        for lib in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm | grep -v "debuginfo" | grep -v "java-1.6.0-sun"); do
          $(rpm -qlp $lib 2>/dev/null | grep "/usr/lib/" | grep -v "/usr/lib/python" > /dev/null) && LINK32PACKS="$LINK32PACKS $(basename $lib)"
        done

	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.i?86.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/i386/debug/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm $RFDIR/$REPO/el/$BRANCH/$VERSION/i386/

        cd $RFDIR/$REPO/el/$BRANCH/$VERSION/x86_64/
        for pack in $LINK32PACKS; do
          ln -vf ../i386/$pack
        done
        cd -

	reindex_updates
	REINDEX="true"
	;;
      updates-testing)
        unset LINK32PACKS
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.src.rpm $RFDIR/$REPO/el/updates/testing/$VERSION/SRPMS/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.x86_64.rpm $RFDIR/$REPO/el/updates/testing/$VERSION/x86_64/debug
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.x86_64.rpm $RFDIR/$REPO/el/updates/testing/$VERSION/x86_64/

	for noarch in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.noarch.rpm); do
	  mv -v $noarch $RFDIR/$REPO/el/updates/testing/$VERSION/i386/
	  cd $RFDIR/$REPO/el/updates/testing/$VERSION/x86_64/
	  ln -vf ../i386/$(basename $noarch)
	  cd -
	done

        for lib in $(ls $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm | grep -v "debuginfo" | grep -v "java-1.6.0-sun"); do
          $(rpm -qlp $lib 2>/dev/null | grep "/usr/lib/" | grep -v "/usr/lib/python" > /dev/null) && LINK32PACKS="$LINK32PACKS $(basename $lib)"
        done

	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*debuginfo*.i?86.rpm $RFDIR/$REPO/el/updates/testing/$VERSION/i386/debug/
	mv -v $SOURCEDIR/$VERSION/$REPO/$BRANCH/*.i?86.rpm $RFDIR/$REPO/el/updates/testing/$VERSION/i386/

        cd $RFDIR/$REPO/el/updates/testing/$VERSION/x86_64/
        for pack in $LINK32PACKS; do
          ln -vf ../i386/$pack
        done
        cd -

	reindex_updates-testing
	REINDEX="true"
	;;
      *)
	exit 1;
    esac
  done
  if [ "$REINDEX" == "true" ]; then
    echo "$(date -u)" > /mirror/fedora/russianfedora/.mirror.yandex.ru
  fi
else
  if [ "$REPO" != "free" -a "$REPO" != "nonfree" -a "$REPO" != "fixes" ]; then
    echo "Not supporting repository. Abort..."
    usage
  fi

  if [ "$VERSION" != "development" -a "$VERSION" != "6" ]; then
    echo "Not supporting version. Abort..."
    usage
  fi

  if [ "$BRANCH" != "" -a "$VERSION" == "development" ]; then
    echo "Do not select branch for development. Abort..."
    exit 1;
  elif [ "$BRANCH" != "" -a "$VERSION" == "pre" ]; then
    echo "Do not select branch for pre. Abort..."
    exit 1;
  elif [ "$BRANCH" == "" -a "$VERSION" == "development" ]; then
    echo OK
  elif [ "$BRANCH" == "" -a "$VERSION" == "pre" ]; then
    echo OK
  elif [ "$BRANCH" != "releases" -a "$BRANCH" != "updates" -a "$BRANCH" != "updates-testing" ]; then
    echo "Not supporting branch. Abort..."
    usage
  fi

  if [ "$VERSION" == "development" ]; then
    reindex_development
  fi

  if [ "$VERSION" == "pre" ]; then
    reindex_pre
  fi

  if [ "$BRANCH" == "releases" ]; then
    reindex_releases
  fi

  if [ "$BRANCH" == "updates" ]; then
    reindex_updates
  fi

  if [ "$BRANCH" == "updates-testing" ]; then
    reindex_updates-testing
  fi

  echo "$(date -u)" > /mirror/fedora/russianfedora/.mirror.yandex.ru
fi
