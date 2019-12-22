#pragma once

#include "hsx.h"
#include "hsp_object_key_fwd.h"

namespace HspObjectKey {
	class Object {
	public:
		virtual ~Object() {
		}

		virtual void accept(ObjectVisitor& visitor) const = 0;
	};

	class ObjectVisitor {
	public:
		virtual ~ObjectVisitor() {
		}

		virtual void on_module(Module const& module) {
		}

		virtual void on_static_var(StaticVar const& static_var) {
		}

		virtual void on_element(Element const& element) {
		}

		virtual void on_int(Int const& value) {
		}
	};

	class ArrayLike {
	public:
		virtual ~ArrayLike() {
		}

		virtual void accept(ArrayLikeVisitor& visitor) const = 0;
	};

	class ArrayLikeVisitor {
	public:
		virtual ~ArrayLikeVisitor() {
		}

		virtual void on_static_var(StaticVar const& static_var) {
		}
	};

	class ElementLike {
	public:
		virtual ~ElementLike() {
		}

		virtual void accept(ElementLikeVisitor& visitor) const = 0;
	};

	class ElementLikeVisitor {
	public:
		virtual ~ElementLikeVisitor() {
		}

		virtual void on_static_var(StaticVar const& static_var) {
		}

		virtual void on_element(Element const& element) {
		}
	};

	class IntLike {
	public:
		virtual ~IntLike() {
		}

		virtual void accept(IntLikeVisitor& visitor) const = 0;
	};

	class IntLikeVisitor {
	public:
		virtual ~IntLikeVisitor() {
		}

		virtual void on_static_var(StaticVar const& static_var) {
		}

		virtual void on_element(Element const& element) {
		}

		virtual void on_int(Int const& value) {
		}
	};

	class Module final
		: public Object
	{
		std::size_t module_id_;

	public:
		explicit Module(std::size_t module_id)
			: module_id_(module_id)
		{
		}

		auto module_id() const -> std::size_t {
			return module_id_;
		}

		void Object::accept(ObjectVisitor& visitor) const {
			visitor.on_module(*this);
		}
	};

	class StaticVar final
		: public Object
		, public ArrayLike
		, public ElementLike
		, public IntLike
	{
		std::size_t static_var_id_;

	public:
		explicit StaticVar(std::size_t static_var_id)
			: static_var_id_(static_var_id)
		{
		}

		auto static_var_id() const -> std::size_t {
			return static_var_id_;
		}

		void Object::accept(ObjectVisitor& visitor) const {
			visitor.on_static_var(*this);
		}

		void ArrayLike::accept(ArrayLikeVisitor& visitor) const {
			visitor.on_static_var(*this);
		}

		void ElementLike::accept(ElementLikeVisitor& visitor) const {
			visitor.on_static_var(*this);
		}

		void IntLike::accept(IntLikeVisitor& visitor) const {
			visitor.on_static_var(*this);
		}
	};

	class Element final
		: public Object
		, public ElementLike
		, public IntLike
	{
		std::shared_ptr<ArrayLike> array_;
		hsx::HspDimIndex indexes_;

	public:
		Element(std::shared_ptr<ArrayLike> array, hsx::HspDimIndex indexes)
			: array_(std::move(array))
			, indexes_(indexes)
		{
		}

		auto array() const -> ArrayLike const& {
			return *array_;
		}

		auto indexes() const -> hsx::HspDimIndex {
			return indexes_;
		}

		void Object::accept(ObjectVisitor& visitor) const {
			visitor.on_element(*this);
		}

		void ElementLike::accept(ElementLikeVisitor& visitor) const {
			visitor.on_element(*this);
		}

		void IntLike::accept(IntLikeVisitor& visitor) const {
			visitor.on_element(*this);
		}
	};

	class Int final
		: public Object
		, public IntLike
	{
		std::shared_ptr<IntLike> source_;

	public:
		explicit Int(std::shared_ptr<IntLike> source)
			: source_(std::move(source))
		{
		}

		void Object::accept(ObjectVisitor& visitor) const {
			visitor.on_int(*this);
		}

		void IntLike::accept(IntLikeVisitor& visitor) const {
			visitor.on_int(*this);
		}
	};
}
