#ifndef __MC_TYPE_HH__
#define __MC_TYPE_HH__

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mc {

struct Type {
    virtual ~Type() = default;

    virtual std::ostream &print(std::ostream &os) const = 0;

    template<class T>
    T *cast() const {
        return dynamic_cast<T *>(this);
    }
    template<class T>
    const T *cast() const {
        return dynamic_cast<const T *>(this);
    }

    virtual int byteSize() const { return -1; }

    std::string signature() const;

    friend std::ostream &operator<<(std::ostream &os, const Type &t);
    friend bool operator==(const Type &a, const Type &b);
};

struct Field {

    Field(std::string s, Type *t) : id(std::move(s)), type(t) {}
    virtual ~Field() { delete (type); }

    friend std::ostream &operator<<(std::ostream &os, const Field &f);

    const std::string id;
    Type *const type;
};

struct PrimitiveType : public Type {
};

struct IntType : public PrimitiveType {
    std::ostream &print(std::ostream &os) const override;

    int byteSize() const override;
};

struct CompoundType : public Type {
};

struct FuncType : public CompoundType {
    std::ostream &print(std::ostream &os) const override;

    FuncType(Type *r, const std::vector<Field *> &fs);
    FuncType(Type *r, std::vector<Type *> ps) : ret(r), params(std::move(ps)) {}
    ~FuncType() override;

    Type *getRet() const { return ret; }
    const std::vector<Type *> &getParams() const { return params; }

private:
    Type *ret;
    std::vector<Type *> params;
};

struct VariantArrayType : public CompoundType {
    explicit VariantArrayType(Type *b) : base(b) {}
    ~VariantArrayType() override;

    std::ostream &print(std::ostream &os) const override;

    Type *getBase() const { return base; }

protected:
    Type *base;
};

struct ArrayType : public VariantArrayType {
    ArrayType(int w, Type *b) : VariantArrayType(b), width(w) {}

    std::ostream &print(std::ostream &os) const override;

    int byteSize() const override;

    int getWidth() const { return width; }

private:
    int width;
};

}; // namespace mc

#endif