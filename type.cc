#include "type.hh"
#include <typeinfo>

namespace mc {

std::ostream &IntType::print(std::ostream &os) const {
    os << "int";
    return os;
}
std::ostream &VariantArrayType::print(std::ostream &os) const {
    os << "[]";
    base->print(os);
    return os;
}
VariantArrayType::~VariantArrayType() {
    delete (base);
}
std::ostream &ArrayType::print(std::ostream &os) const {
    os << '[' << width << ']';
    base->print(os);
    return os;
}
std::ostream &FuncType::print(std::ostream &os) const {
    os << '(';
    for (auto p : params)
        p->print(os) << ", ";
    os << "\b\b)=>";
    ret->print(os);
    return os;
}
FuncType::~FuncType() {
    delete (ret);
    for (auto p: params)
        delete (p);
}
FuncType::FuncType(Type *r, const std::vector<Field *> &fs) : ret(r), params() {
    for (auto f :fs)
        params.push_back(f->type);
}
int IntType::byteSize() const {
    return 4;
}
int ArrayType::byteSize() const {
    int base_size = base->byteSize();
    return base_size < 0 ? base_size : width * base_size;
}
std::ostream &operator<<(std::ostream &os, const Field &f) {
    return os << f.id << ": " << *f.type;
}
std::string Type::signature() const {
    std::ostringstream os;
    print(os);
    return os.str();
}
std::ostream &operator<<(std::ostream &os, const Type &t) {
    return t.print(os);
}
bool operator==(const Type &a, const Type &b) {
    return a.signature() == b.signature();
}

}; // namespace mc