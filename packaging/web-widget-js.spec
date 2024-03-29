Name:          web-widget-js
Version:       0.0.1
Release:       0
Summary:       Web Widget Engine
Source:        %{name}-%{version}.tar.gz
Group:         Development/Libraries
License:       Apache-2.0 and BSD-2.0 and MIT and ICU and LGPL-2.1+

# build requirements
BuildRequires: make
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(icu-uc)

%description
Dummy package of Web Widget JS Engine

%package devel
Summary:    Web Widget JS Engine headers & archives
Group:      Development/Libraries
Requires:   %{name} = %{version}

%description devel
Web Widget JS Engine headers & archives

%prep
%setup -q

# bdwgc fails to compile with 'asan' due to -fcommon flag.
# We exclude this option (as stated in the guide)
%if 0%{?asan:1}
%restore_fcommon
%endif

%build

export ESCARGOT_ARCH=i386
%ifarch %{arm}
export ESCARGOT_ARCH=arm
%endif
%ifarch aarch64
export ESCARGOT_ARCH=arm64
%endif
%ifarch x86_64
export ESCARGOT_ARCH=x64
%endif

%if 0%{?devel_mode}
./build_third_party.sh tizen_obs_${ESCARGOT_ARCH}
make tizen_obs_${ESCARGOT_ARCH}.interpreter.debug.static %{?jobs:-j%jobs}
make tizen_obs_${ESCARGOT_ARCH}.interpreter.debug %{?jobs:-j%jobs}
make tizen_obs_${ESCARGOT_ARCH}.interpreter.release %{?jobs:-j%jobs}
%else
./build_third_party.sh tizen_obs_${ESCARGOT_ARCH} only_release
mkdir -p out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release
mkdir -p out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug
touch out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release/escargot
touch out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug/escargot
touch out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug/libescargot.a
%endif

make tizen_obs_${ESCARGOT_ARCH}.interpreter.release.static %{?jobs:-j%jobs}

%install

export ESCARGOT_ARCH=i386
%ifarch %{arm}
export ESCARGOT_ARCH=arm
%endif
%ifarch aarch64
export ESCARGOT_ARCH=arm64
%endif
%ifarch x86_64
export ESCARGOT_ARCH=x64
%endif

rm -rf %{buildroot}

# Archive
mkdir -p %{buildroot}%{_libdir}/%{name}/release
cp out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release/libescargot.a              %{buildroot}%{_libdir}/%{name}/release
cp third_party/bdwgc/out/tizen_obs/${ESCARGOT_ARCH}/release.shared/.libs/libgc.a %{buildroot}%{_libdir}/%{name}/release

mkdir -p %{buildroot}%{_libdir}/%{name}/debug
cp out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug/libescargot.a              %{buildroot}%{_libdir}/%{name}/debug
cp third_party/bdwgc/out/tizen_obs/${ESCARGOT_ARCH}/debug.shared/.libs/libgc.a %{buildroot}%{_libdir}/%{name}/debug

# Binary
mkdir -p %{buildroot}%{_bindir}/%{name}/release
cp out/tizen_obs/${ESCARGOT_ARCH}/interpreter/release/escargot                   %{buildroot}%{_bindir}/%{name}/release

mkdir -p %{buildroot}%{_bindir}/%{name}/debug
cp out/tizen_obs/${ESCARGOT_ARCH}/interpreter/debug/escargot                   %{buildroot}%{_bindir}/%{name}/debug

# Headers & Build Configurations
LIST=("build" "src/ast" "src/bytecode")
for dir in "${LIST[@]}"; do
echo $dir
done
mkdir -p %{buildroot}%{_includedir}/%{name}/build
mkdir -p %{buildroot}%{_includedir}/%{name}/src/ast
mkdir -p %{buildroot}%{_includedir}/%{name}/src/bytecode
mkdir -p %{buildroot}%{_includedir}/%{name}/src/parser
mkdir -p %{buildroot}%{_includedir}/%{name}/src/runtime
mkdir -p %{buildroot}%{_includedir}/%{name}/src/vm
mkdir -p %{buildroot}%{_includedir}/%{name}/third_party/bdwgc
mkdir -p %{buildroot}%{_includedir}/%{name}/third_party/checked_arithmetic
mkdir -p %{buildroot}%{_includedir}/%{name}/third_party/double_conversion
mkdir -p %{buildroot}%{_includedir}/%{name}/third_party/rapidjson
mkdir -p %{buildroot}%{_includedir}/%{name}/third_party/yarr
cp build/*.mk %{buildroot}%{_includedir}/%{name}/build/
cp src/*.h %{buildroot}%{_includedir}/%{name}/src
cp src/ast/*.h %{buildroot}%{_includedir}/%{name}/src/ast
cp src/bytecode/*.h %{buildroot}%{_includedir}/%{name}/src/bytecode
cp src/parser/*.h %{buildroot}%{_includedir}/%{name}/src/parser
cp src/runtime/*.h %{buildroot}%{_includedir}/%{name}/src/runtime
cp src/vm/*.h %{buildroot}%{_includedir}/%{name}/src/vm
cp -r third_party/bdwgc/include %{buildroot}%{_includedir}/%{name}/third_party/bdwgc
cp third_party/checked_arithmetic/*.h %{buildroot}%{_includedir}/%{name}/third_party/checked_arithmetic
cp third_party/double_conversion/*.h %{buildroot}%{_includedir}/%{name}/third_party/double_conversion
cp -r third_party/rapidjson/include %{buildroot}%{_includedir}/%{name}/third_party/rapidjson
cp third_party/yarr/*.h %{buildroot}%{_includedir}/%{name}/third_party/yarr

%files
%license LICENSE LICENSE.BSD-2.0 LICENSE.ICU LICENSE.LGPL-2.1+ LICENSE.MIT COPYING.LIB

%files devel
%{_includedir}/%{name}
%{_libdir}/%{name}/*/*.a
%{_bindir}/%{name}/*/escargot
