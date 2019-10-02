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
bool VariantArrayType::equal(const Type &other) const {
    auto rhs = dynamic_cast<const VariantArrayType &>(other);
    return *base == *rhs.base;
}
Type *VariantArrayType::indexType(int dim) {
    return dim == 0 ? this : base->indexType(dim - 1);
}
bool VariantArrayType::compatible(const Type &other) const {
    if (typeid(other) == typeid(ArrayType)) {
        return *base == *dynamic_cast<const ArrayType &>(other).base;
    } else {
        return *this == other;
    }
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
    os << ")=>";
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
bool FuncType::equal(const Type &other) const {
    auto rhs = dynamic_cast<const FuncType &>(other);
    if (*rhs.ret != *ret)
        return false;
    size_t len = params.size();
    if (len != rhs.params.size())
        return false;
    for (size_t i = 0; i < len; ++i) {
        if (*params[i] != *rhs.params[i])
            return false;
    }
    return true;
}
int IntType::byteSize() const {
    return 4;
}
bool IntType::equal(const Type &other) const {
    return true;
}
int ArrayType::byteSize() const {
    int base_size = base->byteSize();
    return base_size < 0 ? base_size : width * base_size;
}
bool ArrayType::equal(const Type &other) const {
    auto rhs = dynamic_cast<const ArrayType &>(other);
    return width == rhs.width && *base == *rhs.base;
}
std::ostream &operator<<(std::ostream &os, const Field &f) {
    return os << f.id << ": " << *f.type;
}
std::ostream &operator<<(std::ostream &os, const Type &t) {
    return t.print(os);
}
bool operator==(const Type &a, const Type &b) {
    return typeid(a) == typeid(b) && a.equal(b);
}
bool operator!=(const Type &a, const Type &b) {
    return !(a == b);
}
Type *Type::indexType(int dim) {
    return dim == 0 ? this : nullptr;
}

}; // namespace mc