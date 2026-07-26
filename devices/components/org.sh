set -eu

dir="${1:-}"
if [ -z "$dir" ]; then 
    echo "Usage: $0 <directory>" >&2
    exit 1
fi

idf=1idf_component_register(
    SRCS ""
    INCLUDE_DIRS "include"
)1

cd $dir
printf '%s\n' "$idf" > CMakeLists.txt

mkdir -p include

for f in ./*.hpp; do
    [ -e "$f" ] || break
    mv -- "$f" include/
done
