#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Data.dll>
#using <System.Xml.dll>

using namespace System::Security::Permissions;
[assembly:SecurityPermissionAttribute(SecurityAction::RequestMinimum, SkipVerification=false)];
// 
// Этот исходный код был создан с помощью xsd, версия=4.6.1055.0.
// 
namespace FirmwareUpdater {
    using namespace System;
    ref class NewDataSet;
    
    
    /// <summary>
///Represents a strongly typed in-memory cache of data.
///</summary>
    [System::Serializable, 
    System::ComponentModel::DesignerCategoryAttribute(L"code"), 
    System::ComponentModel::ToolboxItem(true), 
    System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedDataSetSchema"), 
    System::Xml::Serialization::XmlRootAttribute(L"NewDataSet"), 
    System::ComponentModel::Design::HelpKeywordAttribute(L"vs.data.DataSet")]
    public ref class NewDataSet : public ::System::Data::DataSet {
        public : ref class hostDataTable;
        public : ref class hostListDataTable;
        public : ref class groupDataTable;
        public : ref class hostRow;
        public : ref class hostListRow;
        public : ref class groupRow;
        public : ref class hostRowChangeEvent;
        public : ref class hostListRowChangeEvent;
        public : ref class groupRowChangeEvent;
        
        private: FirmwareUpdater::NewDataSet::hostDataTable^  tablehost;
        
        private: FirmwareUpdater::NewDataSet::hostListDataTable^  tablehostList;
        
        private: FirmwareUpdater::NewDataSet::groupDataTable^  tablegroup;
        
        private: ::System::Data::DataRelation^  relationgroup_host;
        
        private: ::System::Data::DataRelation^  relationhostList_group;
        
        private: ::System::Data::SchemaSerializationMode _schemaSerializationMode;
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void hostRowChangeEventHandler(::System::Object^  sender, FirmwareUpdater::NewDataSet::hostRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void hostListRowChangeEventHandler(::System::Object^  sender, FirmwareUpdater::NewDataSet::hostListRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void groupRowChangeEventHandler(::System::Object^  sender, FirmwareUpdater::NewDataSet::groupRowChangeEvent^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        NewDataSet();
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        NewDataSet(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property FirmwareUpdater::NewDataSet::hostDataTable^  host {
            FirmwareUpdater::NewDataSet::hostDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property FirmwareUpdater::NewDataSet::hostListDataTable^  hostList {
            FirmwareUpdater::NewDataSet::hostListDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property FirmwareUpdater::NewDataSet::groupDataTable^  group {
            FirmwareUpdater::NewDataSet::groupDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::BrowsableAttribute(true), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Visible)]
        virtual property ::System::Data::SchemaSerializationMode SchemaSerializationMode {
            ::System::Data::SchemaSerializationMode get() override;
            System::Void set(::System::Data::SchemaSerializationMode value) override;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataTableCollection^  Tables {
            ::System::Data::DataTableCollection^  get() new;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataRelationCollection^  Relations {
            ::System::Data::DataRelationCollection^  get() new;
        }
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Void InitializeDerivedDataSet() override;
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Data::DataSet^  Clone() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Boolean ShouldSerializeTables() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Boolean ShouldSerializeRelations() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Void ReadXmlSerializable(::System::Xml::XmlReader^  reader) override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Xml::Schema::XmlSchema^  GetSchemaSerializable() override;
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitVars();
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitVars(::System::Boolean initTable);
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitClass();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializehost();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializehostList();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializegroup();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class hostDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnipAddress;
            
            private: ::System::Data::DataColumn^  columnpassword;
            
            private: ::System::Data::DataColumn^  columninfo;
            
            private: ::System::Data::DataColumn^  columngroup_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostRowChangeEventHandler^  hostRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostRowChangeEventHandler^  hostRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostRowChangeEventHandler^  hostRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostRowChangeEventHandler^  hostRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  ipAddressColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  passwordColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  infoColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  group_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::hostRow^  default [::System::Int32 ] {
                FirmwareUpdater::NewDataSet::hostRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddhostRow(FirmwareUpdater::NewDataSet::hostRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::hostRow^  AddhostRow(System::String^  ipAddress, System::String^  password, System::String^  info, 
                        FirmwareUpdater::NewDataSet::groupRow^  parentgroupRowBygroup_host);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::hostRow^  NewhostRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovehostRow(FirmwareUpdater::NewDataSet::hostRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class hostListDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnhostList_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostListRowChangeEventHandler^  hostListRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostListRowChangeEventHandler^  hostListRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostListRowChangeEventHandler^  hostListRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::hostListRowChangeEventHandler^  hostListRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostListDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostListDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostListDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  hostList_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::hostListRow^  default [::System::Int32 ] {
                FirmwareUpdater::NewDataSet::hostListRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddhostListRow(FirmwareUpdater::NewDataSet::hostListRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::hostListRow^  AddhostListRow();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::hostListRow^  NewhostListRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovehostListRow(FirmwareUpdater::NewDataSet::hostListRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class groupDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columntitle;
            
            private: ::System::Data::DataColumn^  columngroup_Id;
            
            private: ::System::Data::DataColumn^  columnhostList_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::groupRowChangeEventHandler^  groupRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::groupRowChangeEventHandler^  groupRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::groupRowChangeEventHandler^  groupRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event FirmwareUpdater::NewDataSet::groupRowChangeEventHandler^  groupRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            groupDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            groupDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            groupDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  titleColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  group_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  hostList_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::groupRow^  default [::System::Int32 ] {
                FirmwareUpdater::NewDataSet::groupRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddgroupRow(FirmwareUpdater::NewDataSet::groupRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::groupRow^  AddgroupRow(System::String^  title, FirmwareUpdater::NewDataSet::hostListRow^  parenthostListRowByhostList_group);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            FirmwareUpdater::NewDataSet::groupRow^  NewgroupRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovegroupRow(FirmwareUpdater::NewDataSet::groupRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class hostRow : public ::System::Data::DataRow {
            
            private: FirmwareUpdater::NewDataSet::hostDataTable^  tablehost;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  ipAddress {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  password {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  info {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 group_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::groupRow^  groupRow {
                FirmwareUpdater::NewDataSet::groupRow^  get();
                System::Void set(FirmwareUpdater::NewDataSet::groupRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsinfoNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetinfoNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isgroup_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setgroup_IdNull();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class hostListRow : public ::System::Data::DataRow {
            
            private: FirmwareUpdater::NewDataSet::hostListDataTable^  tablehostList;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostListRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 hostList_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< FirmwareUpdater::NewDataSet::groupRow^  >^  GetgroupRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class groupRow : public ::System::Data::DataRow {
            
            private: FirmwareUpdater::NewDataSet::groupDataTable^  tablegroup;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            groupRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  title {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 group_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 hostList_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::hostListRow^  hostListRow {
                FirmwareUpdater::NewDataSet::hostListRow^  get();
                System::Void set(FirmwareUpdater::NewDataSet::hostListRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IstitleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SettitleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IshostList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SethostList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< FirmwareUpdater::NewDataSet::hostRow^  >^  GethostRows();
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class hostRowChangeEvent : public ::System::EventArgs {
            
            private: FirmwareUpdater::NewDataSet::hostRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostRowChangeEvent(FirmwareUpdater::NewDataSet::hostRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::hostRow^  Row {
                FirmwareUpdater::NewDataSet::hostRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class hostListRowChangeEvent : public ::System::EventArgs {
            
            private: FirmwareUpdater::NewDataSet::hostListRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            hostListRowChangeEvent(FirmwareUpdater::NewDataSet::hostListRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::hostListRow^  Row {
                FirmwareUpdater::NewDataSet::hostListRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class groupRowChangeEvent : public ::System::EventArgs {
            
            private: FirmwareUpdater::NewDataSet::groupRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            groupRowChangeEvent(FirmwareUpdater::NewDataSet::groupRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property FirmwareUpdater::NewDataSet::groupRow^  Row {
                FirmwareUpdater::NewDataSet::groupRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
    };
}
namespace FirmwareUpdater {
    
    
    inline NewDataSet::NewDataSet() {
        this->BeginInit();
        this->InitClass();
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &FirmwareUpdater::NewDataSet::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        __super::Relations->CollectionChanged += schemaChangedHandler;
        this->EndInit();
    }
    
    inline NewDataSet::NewDataSet(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataSet(info, context, false) {
        if (this->IsBinarySerialized(info, context) == true) {
            this->InitVars(false);
            ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler1 = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &FirmwareUpdater::NewDataSet::SchemaChanged);
            this->Tables->CollectionChanged += schemaChangedHandler1;
            this->Relations->CollectionChanged += schemaChangedHandler1;
            return;
        }
        ::System::String^  strSchema = (cli::safe_cast<::System::String^  >(info->GetValue(L"XmlSchema", ::System::String::typeid)));
        if (this->DetermineSchemaSerializationMode(info, context) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
            if (ds->Tables[L"host"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::hostDataTable(ds->Tables[L"host"])));
            }
            if (ds->Tables[L"hostList"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::hostListDataTable(ds->Tables[L"hostList"])));
            }
            if (ds->Tables[L"group"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::groupDataTable(ds->Tables[L"group"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
        }
        this->GetSerializationData(info, context);
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &FirmwareUpdater::NewDataSet::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        this->Relations->CollectionChanged += schemaChangedHandler;
    }
    
    inline FirmwareUpdater::NewDataSet::hostDataTable^  NewDataSet::host::get() {
        return this->tablehost;
    }
    
    inline FirmwareUpdater::NewDataSet::hostListDataTable^  NewDataSet::hostList::get() {
        return this->tablehostList;
    }
    
    inline FirmwareUpdater::NewDataSet::groupDataTable^  NewDataSet::group::get() {
        return this->tablegroup;
    }
    
    inline ::System::Data::SchemaSerializationMode NewDataSet::SchemaSerializationMode::get() {
        return this->_schemaSerializationMode;
    }
    inline System::Void NewDataSet::SchemaSerializationMode::set(::System::Data::SchemaSerializationMode value) {
        this->_schemaSerializationMode = __identifier(value);
    }
    
    inline ::System::Data::DataTableCollection^  NewDataSet::Tables::get() {
        return __super::Tables;
    }
    
    inline ::System::Data::DataRelationCollection^  NewDataSet::Relations::get() {
        return __super::Relations;
    }
    
    inline ::System::Void NewDataSet::InitializeDerivedDataSet() {
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline ::System::Data::DataSet^  NewDataSet::Clone() {
        FirmwareUpdater::NewDataSet^  cln = (cli::safe_cast<FirmwareUpdater::NewDataSet^  >(__super::Clone()));
        cln->InitVars();
        cln->SchemaSerializationMode = this->SchemaSerializationMode;
        return cln;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeTables() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeRelations() {
        return false;
    }
    
    inline ::System::Void NewDataSet::ReadXmlSerializable(::System::Xml::XmlReader^  reader) {
        if (this->DetermineSchemaSerializationMode(reader) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            this->Reset();
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXml(reader);
            if (ds->Tables[L"host"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::hostDataTable(ds->Tables[L"host"])));
            }
            if (ds->Tables[L"hostList"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::hostListDataTable(ds->Tables[L"hostList"])));
            }
            if (ds->Tables[L"group"] != nullptr) {
                __super::Tables->Add((gcnew FirmwareUpdater::NewDataSet::groupDataTable(ds->Tables[L"group"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXml(reader);
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchema^  NewDataSet::GetSchemaSerializable() {
        ::System::IO::MemoryStream^  stream = (gcnew ::System::IO::MemoryStream());
        this->WriteXmlSchema((gcnew ::System::Xml::XmlTextWriter(stream, nullptr)));
        stream->Position = 0;
        return ::System::Xml::Schema::XmlSchema::Read((gcnew ::System::Xml::XmlTextReader(stream)), nullptr);
    }
    
    inline ::System::Void NewDataSet::InitVars() {
        this->InitVars(true);
    }
    
    inline ::System::Void NewDataSet::InitVars(::System::Boolean initTable) {
        this->tablehost = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostDataTable^  >(__super::Tables[L"host"]));
        if (initTable == true) {
            if (this->tablehost != nullptr) {
                this->tablehost->InitVars();
            }
        }
        this->tablehostList = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListDataTable^  >(__super::Tables[L"hostList"]));
        if (initTable == true) {
            if (this->tablehostList != nullptr) {
                this->tablehostList->InitVars();
            }
        }
        this->tablegroup = (cli::safe_cast<FirmwareUpdater::NewDataSet::groupDataTable^  >(__super::Tables[L"group"]));
        if (initTable == true) {
            if (this->tablegroup != nullptr) {
                this->tablegroup->InitVars();
            }
        }
        this->relationgroup_host = this->Relations[L"group_host"];
        this->relationhostList_group = this->Relations[L"hostList_group"];
    }
    
    inline ::System::Void NewDataSet::InitClass() {
        this->DataSetName = L"NewDataSet";
        this->Prefix = L"";
        this->Locale = (gcnew ::System::Globalization::CultureInfo(L""));
        this->EnforceConstraints = true;
        this->SchemaSerializationMode = ::System::Data::SchemaSerializationMode::IncludeSchema;
        this->tablehost = (gcnew FirmwareUpdater::NewDataSet::hostDataTable());
        __super::Tables->Add(this->tablehost);
        this->tablehostList = (gcnew FirmwareUpdater::NewDataSet::hostListDataTable());
        __super::Tables->Add(this->tablehostList);
        this->tablegroup = (gcnew FirmwareUpdater::NewDataSet::groupDataTable());
        __super::Tables->Add(this->tablegroup);
        ::System::Data::ForeignKeyConstraint^  fkc;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"group_host", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablegroup->group_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablehost->group_IdColumn}));
        this->tablehost->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"hostList_group", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablehostList->hostList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablegroup->hostList_IdColumn}));
        this->tablegroup->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        this->relationgroup_host = (gcnew ::System::Data::DataRelation(L"group_host", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablegroup->group_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablehost->group_IdColumn}, false));
        this->relationgroup_host->Nested = true;
        this->Relations->Add(this->relationgroup_host);
        this->relationhostList_group = (gcnew ::System::Data::DataRelation(L"hostList_group", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablehostList->hostList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablegroup->hostList_IdColumn}, false));
        this->relationhostList_group->Nested = true;
        this->Relations->Add(this->relationhostList_group);
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializehost() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializehostList() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializegroup() {
        return false;
    }
    
    inline ::System::Void NewDataSet::SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e) {
        if (e->Action == ::System::ComponentModel::CollectionChangeAction::Remove) {
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        FirmwareUpdater::NewDataSet^  ds = (gcnew FirmwareUpdater::NewDataSet());
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        ::System::Xml::Schema::XmlSchemaAny^  any = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any->Namespace = ds->Namespace;
        sequence->Items->Add(any);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::hostDataTable::hostDataTable() {
        this->TableName = L"host";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::hostDataTable::hostDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::hostDataTable::hostDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::hostDataTable::ipAddressColumn::get() {
        return this->columnipAddress;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::hostDataTable::passwordColumn::get() {
        return this->columnpassword;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::hostDataTable::infoColumn::get() {
        return this->columninfo;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::hostDataTable::group_IdColumn::get() {
        return this->columngroup_Id;
    }
    
    inline ::System::Int32 NewDataSet::hostDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline FirmwareUpdater::NewDataSet::hostRow^  NewDataSet::hostDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::hostDataTable::AddhostRow(FirmwareUpdater::NewDataSet::hostRow^  row) {
        this->Rows->Add(row);
    }
    
    inline FirmwareUpdater::NewDataSet::hostRow^  NewDataSet::hostDataTable::AddhostRow(System::String^  ipAddress, System::String^  password, 
                System::String^  info, FirmwareUpdater::NewDataSet::groupRow^  parentgroupRowBygroup_host) {
        FirmwareUpdater::NewDataSet::hostRow^  rowhostRow = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(4) {ipAddress, password, 
            info, nullptr};
        if (parentgroupRowBygroup_host != nullptr) {
            columnValuesArray[3] = parentgroupRowBygroup_host[1];
        }
        rowhostRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowhostRow);
        return rowhostRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::hostDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::hostDataTable::Clone() {
        FirmwareUpdater::NewDataSet::hostDataTable^  cln = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::hostDataTable::CreateInstance() {
        return (gcnew FirmwareUpdater::NewDataSet::hostDataTable());
    }
    
    inline ::System::Void NewDataSet::hostDataTable::InitVars() {
        this->columnipAddress = __super::Columns[L"ipAddress"];
        this->columnpassword = __super::Columns[L"password"];
        this->columninfo = __super::Columns[L"info"];
        this->columngroup_Id = __super::Columns[L"group_Id"];
    }
    
    inline ::System::Void NewDataSet::hostDataTable::InitClass() {
        this->columnipAddress = (gcnew ::System::Data::DataColumn(L"ipAddress", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnipAddress);
        this->columnpassword = (gcnew ::System::Data::DataColumn(L"password", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnpassword);
        this->columninfo = (gcnew ::System::Data::DataColumn(L"info", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columninfo);
        this->columngroup_Id = (gcnew ::System::Data::DataColumn(L"group_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columngroup_Id);
        this->columnipAddress->AllowDBNull = false;
        this->columnipAddress->Namespace = L"";
        this->columnpassword->AllowDBNull = false;
        this->columnpassword->Namespace = L"";
        this->columninfo->Namespace = L"";
    }
    
    inline FirmwareUpdater::NewDataSet::hostRow^  NewDataSet::hostDataTable::NewhostRow() {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::hostDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew FirmwareUpdater::NewDataSet::hostRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::hostDataTable::GetRowType() {
        return FirmwareUpdater::NewDataSet::hostRow::typeid;
    }
    
    inline ::System::Void NewDataSet::hostDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->hostRowChanged(this, (gcnew FirmwareUpdater::NewDataSet::hostRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->hostRowChanging(this, (gcnew FirmwareUpdater::NewDataSet::hostRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->hostRowDeleted(this, (gcnew FirmwareUpdater::NewDataSet::hostRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->hostRowDeleting(this, (gcnew FirmwareUpdater::NewDataSet::hostRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostDataTable::RemovehostRow(FirmwareUpdater::NewDataSet::hostRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::hostDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        FirmwareUpdater::NewDataSet^  ds = (gcnew FirmwareUpdater::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"hostDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::hostListDataTable::hostListDataTable() {
        this->TableName = L"hostList";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::hostListDataTable::hostListDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::hostListDataTable::hostListDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::hostListDataTable::hostList_IdColumn::get() {
        return this->columnhostList_Id;
    }
    
    inline ::System::Int32 NewDataSet::hostListDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline FirmwareUpdater::NewDataSet::hostListRow^  NewDataSet::hostListDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::AddhostListRow(FirmwareUpdater::NewDataSet::hostListRow^  row) {
        this->Rows->Add(row);
    }
    
    inline FirmwareUpdater::NewDataSet::hostListRow^  NewDataSet::hostListDataTable::AddhostListRow() {
        FirmwareUpdater::NewDataSet::hostListRow^  rowhostListRow = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(1) {nullptr};
        rowhostListRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowhostListRow);
        return rowhostListRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::hostListDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::hostListDataTable::Clone() {
        FirmwareUpdater::NewDataSet::hostListDataTable^  cln = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::hostListDataTable::CreateInstance() {
        return (gcnew FirmwareUpdater::NewDataSet::hostListDataTable());
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::InitVars() {
        this->columnhostList_Id = __super::Columns[L"hostList_Id"];
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::InitClass() {
        this->columnhostList_Id = (gcnew ::System::Data::DataColumn(L"hostList_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnhostList_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnhostList_Id}, 
                true)));
        this->columnhostList_Id->AutoIncrement = true;
        this->columnhostList_Id->AllowDBNull = false;
        this->columnhostList_Id->Unique = true;
    }
    
    inline FirmwareUpdater::NewDataSet::hostListRow^  NewDataSet::hostListDataTable::NewhostListRow() {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::hostListDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew FirmwareUpdater::NewDataSet::hostListRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::hostListDataTable::GetRowType() {
        return FirmwareUpdater::NewDataSet::hostListRow::typeid;
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->hostListRowChanged(this, (gcnew FirmwareUpdater::NewDataSet::hostListRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->hostListRowChanging(this, (gcnew FirmwareUpdater::NewDataSet::hostListRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->hostListRowDeleted(this, (gcnew FirmwareUpdater::NewDataSet::hostListRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->hostListRowDeleting(this, (gcnew FirmwareUpdater::NewDataSet::hostListRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::hostListDataTable::RemovehostListRow(FirmwareUpdater::NewDataSet::hostListRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::hostListDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        FirmwareUpdater::NewDataSet^  ds = (gcnew FirmwareUpdater::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"hostListDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::groupDataTable::groupDataTable() {
        this->TableName = L"group";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::groupDataTable::groupDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::groupDataTable::groupDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::groupDataTable::titleColumn::get() {
        return this->columntitle;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::groupDataTable::group_IdColumn::get() {
        return this->columngroup_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::groupDataTable::hostList_IdColumn::get() {
        return this->columnhostList_Id;
    }
    
    inline ::System::Int32 NewDataSet::groupDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline FirmwareUpdater::NewDataSet::groupRow^  NewDataSet::groupDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::groupDataTable::AddgroupRow(FirmwareUpdater::NewDataSet::groupRow^  row) {
        this->Rows->Add(row);
    }
    
    inline FirmwareUpdater::NewDataSet::groupRow^  NewDataSet::groupDataTable::AddgroupRow(System::String^  title, FirmwareUpdater::NewDataSet::hostListRow^  parenthostListRowByhostList_group) {
        FirmwareUpdater::NewDataSet::groupRow^  rowgroupRow = (cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(3) {title, nullptr, nullptr};
        if (parenthostListRowByhostList_group != nullptr) {
            columnValuesArray[2] = parenthostListRowByhostList_group[0];
        }
        rowgroupRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowgroupRow);
        return rowgroupRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::groupDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::groupDataTable::Clone() {
        FirmwareUpdater::NewDataSet::groupDataTable^  cln = (cli::safe_cast<FirmwareUpdater::NewDataSet::groupDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::groupDataTable::CreateInstance() {
        return (gcnew FirmwareUpdater::NewDataSet::groupDataTable());
    }
    
    inline ::System::Void NewDataSet::groupDataTable::InitVars() {
        this->columntitle = __super::Columns[L"title"];
        this->columngroup_Id = __super::Columns[L"group_Id"];
        this->columnhostList_Id = __super::Columns[L"hostList_Id"];
    }
    
    inline ::System::Void NewDataSet::groupDataTable::InitClass() {
        this->columntitle = (gcnew ::System::Data::DataColumn(L"title", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columntitle);
        this->columngroup_Id = (gcnew ::System::Data::DataColumn(L"group_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columngroup_Id);
        this->columnhostList_Id = (gcnew ::System::Data::DataColumn(L"hostList_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnhostList_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columngroup_Id}, 
                true)));
        this->columntitle->Namespace = L"";
        this->columngroup_Id->AutoIncrement = true;
        this->columngroup_Id->AllowDBNull = false;
        this->columngroup_Id->Unique = true;
    }
    
    inline FirmwareUpdater::NewDataSet::groupRow^  NewDataSet::groupDataTable::NewgroupRow() {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::groupDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew FirmwareUpdater::NewDataSet::groupRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::groupDataTable::GetRowType() {
        return FirmwareUpdater::NewDataSet::groupRow::typeid;
    }
    
    inline ::System::Void NewDataSet::groupDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->groupRowChanged(this, (gcnew FirmwareUpdater::NewDataSet::groupRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::groupDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->groupRowChanging(this, (gcnew FirmwareUpdater::NewDataSet::groupRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::groupDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->groupRowDeleted(this, (gcnew FirmwareUpdater::NewDataSet::groupRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::groupDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->groupRowDeleting(this, (gcnew FirmwareUpdater::NewDataSet::groupRowChangeEvent((cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::groupDataTable::RemovegroupRow(FirmwareUpdater::NewDataSet::groupRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::groupDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        FirmwareUpdater::NewDataSet^  ds = (gcnew FirmwareUpdater::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"groupDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::hostRow::hostRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablehost = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::hostRow::ipAddress::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tablehost->ipAddressColumn]));
    }
    inline System::Void NewDataSet::hostRow::ipAddress::set(System::String^  value) {
        this[this->tablehost->ipAddressColumn] = value;
    }
    
    inline System::String^  NewDataSet::hostRow::password::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tablehost->passwordColumn]));
    }
    inline System::Void NewDataSet::hostRow::password::set(System::String^  value) {
        this[this->tablehost->passwordColumn] = value;
    }
    
    inline System::String^  NewDataSet::hostRow::info::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tablehost->infoColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'info\' в таблице \'host\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::hostRow::info::set(System::String^  value) {
        this[this->tablehost->infoColumn] = value;
    }
    
    inline System::Int32 NewDataSet::hostRow::group_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablehost->group_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'group_Id\' в таблице \'host\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::hostRow::group_Id::set(System::Int32 value) {
        this[this->tablehost->group_IdColumn] = value;
    }
    
    inline FirmwareUpdater::NewDataSet::groupRow^  NewDataSet::hostRow::groupRow::get() {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::groupRow^  >(this->GetParentRow(this->Table->ParentRelations[L"group_host"])));
    }
    inline System::Void NewDataSet::hostRow::groupRow::set(FirmwareUpdater::NewDataSet::groupRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"group_host"]);
    }
    
    inline ::System::Boolean NewDataSet::hostRow::IsinfoNull() {
        return this->IsNull(this->tablehost->infoColumn);
    }
    
    inline ::System::Void NewDataSet::hostRow::SetinfoNull() {
        this[this->tablehost->infoColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::hostRow::Isgroup_IdNull() {
        return this->IsNull(this->tablehost->group_IdColumn);
    }
    
    inline ::System::Void NewDataSet::hostRow::Setgroup_IdNull() {
        this[this->tablehost->group_IdColumn] = ::System::Convert::DBNull;
    }
    
    
    inline NewDataSet::hostListRow::hostListRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablehostList = (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::hostListRow::hostList_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tablehostList->hostList_IdColumn]));
    }
    inline System::Void NewDataSet::hostListRow::hostList_Id::set(System::Int32 value) {
        this[this->tablehostList->hostList_IdColumn] = value;
    }
    
    inline cli::array< FirmwareUpdater::NewDataSet::groupRow^  >^  NewDataSet::hostListRow::GetgroupRows() {
        if (this->Table->ChildRelations[L"hostList_group"] == nullptr) {
            return gcnew cli::array< FirmwareUpdater::NewDataSet::groupRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< FirmwareUpdater::NewDataSet::groupRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"hostList_group"])));
        }
    }
    
    
    inline NewDataSet::groupRow::groupRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablegroup = (cli::safe_cast<FirmwareUpdater::NewDataSet::groupDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::groupRow::title::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tablegroup->titleColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'title\' в таблице \'group\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::groupRow::title::set(System::String^  value) {
        this[this->tablegroup->titleColumn] = value;
    }
    
    inline System::Int32 NewDataSet::groupRow::group_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tablegroup->group_IdColumn]));
    }
    inline System::Void NewDataSet::groupRow::group_Id::set(System::Int32 value) {
        this[this->tablegroup->group_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::groupRow::hostList_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablegroup->hostList_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'hostList_Id\' в таблице \'group\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::groupRow::hostList_Id::set(System::Int32 value) {
        this[this->tablegroup->hostList_IdColumn] = value;
    }
    
    inline FirmwareUpdater::NewDataSet::hostListRow^  NewDataSet::groupRow::hostListRow::get() {
        return (cli::safe_cast<FirmwareUpdater::NewDataSet::hostListRow^  >(this->GetParentRow(this->Table->ParentRelations[L"hostList_group"])));
    }
    inline System::Void NewDataSet::groupRow::hostListRow::set(FirmwareUpdater::NewDataSet::hostListRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"hostList_group"]);
    }
    
    inline ::System::Boolean NewDataSet::groupRow::IstitleNull() {
        return this->IsNull(this->tablegroup->titleColumn);
    }
    
    inline ::System::Void NewDataSet::groupRow::SettitleNull() {
        this[this->tablegroup->titleColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::groupRow::IshostList_IdNull() {
        return this->IsNull(this->tablegroup->hostList_IdColumn);
    }
    
    inline ::System::Void NewDataSet::groupRow::SethostList_IdNull() {
        this[this->tablegroup->hostList_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< FirmwareUpdater::NewDataSet::hostRow^  >^  NewDataSet::groupRow::GethostRows() {
        if (this->Table->ChildRelations[L"group_host"] == nullptr) {
            return gcnew cli::array< FirmwareUpdater::NewDataSet::hostRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< FirmwareUpdater::NewDataSet::hostRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"group_host"])));
        }
    }
    
    
    inline NewDataSet::hostRowChangeEvent::hostRowChangeEvent(FirmwareUpdater::NewDataSet::hostRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline FirmwareUpdater::NewDataSet::hostRow^  NewDataSet::hostRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::hostRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::hostListRowChangeEvent::hostListRowChangeEvent(FirmwareUpdater::NewDataSet::hostListRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline FirmwareUpdater::NewDataSet::hostListRow^  NewDataSet::hostListRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::hostListRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::groupRowChangeEvent::groupRowChangeEvent(FirmwareUpdater::NewDataSet::groupRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline FirmwareUpdater::NewDataSet::groupRow^  NewDataSet::groupRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::groupRowChangeEvent::Action::get() {
        return this->eventAction;
    }
}
