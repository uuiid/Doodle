from os import getenv
from sqlalchemy import create_engine
from sqlalchemy.orm import Session
import os

import zou.app
import zou.app.models.project_status
import zou.app.models.organisation
import zou.app.models.studio
import zou.app.models.status_automation
import zou.app.models.entity_type
import zou.app.models.task_status
import zou.app.models.task_type
import zou.app.models.department
import zou.app.models.preview_background_file
import zou.app.models.person
import zou.app.models.metadata_descriptor
import zou.app.models.project
import zou.app.models.entity
import zou.app.models.notification
import zou.app.models.subscription

import doodle.project_status
import doodle.organisation
import doodle.studio
import doodle.status_automation
import doodle.entity_type
import doodle.task_status
import doodle.department
import doodle.task_type
import doodle.base
import doodle.preview_background_file
import doodle.person
import doodle.metadata_descriptor
import doodle.project
import doodle.entity
import doodle.asset_instance
import doodle.preview_file
import doodle.software
import doodle.working_file
import doodle.output_file
import doodle.file_status
import doodle.output_type
import doodle.task
import doodle.notification
import doodle.comment
import doodle.attachment_file
import doodle.chat_message
import doodle.chat
import doodle.subscription

PASS = getenv("KITSU_PASS")
DB_NAME = "my_zou1.database"
def main():
    engine = create_engine(f"sqlite:///D:\\{DB_NAME}")

    with zou.app.app.app_context():
        l_prj_list : list[zou.app.models.project_status.ProjectStatus]= zou.app.models.project_status.ProjectStatus.query.all()
        l_organisation : list[zou.app.models.organisation.Organisation] = zou.app.models.organisation.Organisation.query.all()
        l_studio : list[zou.app.models.studio.Studio] = zou.app.models.studio.Studio.query.all()
        l_status_automation : list[zou.app.models.status_automation.StatusAutomation] = zou.app.models.status_automation.StatusAutomation.query.all()
        l_entity_type : list[zou.app.models.entity_type.EntityType] = zou.app.models.entity_type.EntityType.query.all()
        l_TaskTypeAssetTypeLink : list[zou.app.models.entity_type.TaskTypeAssetTypeLink] = zou.app.models.entity_type.TaskTypeAssetTypeLink.query.all()
        l_task_status : list[zou.app.models.task_status.TaskStatus] = zou.app.models.task_status.TaskStatus.query.all()
        l_task_type : list[zou.app.models.task_type.TaskType] = zou.app.models.task_type.TaskType.query.all()
        l_department : list[zou.app.models.department.Department] = zou.app.models.department.Department.query.all()
        l_preview_background_file : list[zou.app.models.preview_background_file.PreviewBackgroundFile] = zou.app.models.preview_background_file.PreviewBackgroundFile.query.all()
        l_person : list[zou.app.models.person.Person] = zou.app.models.person.Person.query.all()
        l_DepartmentLink : list[zou.app.models.person.DepartmentLink] = zou.app.models.person.DepartmentLink.query.all()
        l_metadata_descriptor : list[zou.app.models.metadata_descriptor.MetadataDescriptor] = zou.app.models.metadata_descriptor.MetadataDescriptor.query.all()
        l_DepartmentMetadataDescriptorLink : list[zou.app.models.metadata_descriptor.DepartmentMetadataDescriptorLink] = zou.app.models.metadata_descriptor.DepartmentMetadataDescriptorLink.query.all()
        l_project : list[zou.app.models.project.Project] = zou.app.models.project.Project.query.all()
        l_ProjectPreviewBackgroundFileLink : list[zou.app.models.project.ProjectPreviewBackgroundFileLink] = zou.app.models.project.ProjectPreviewBackgroundFileLink.query.all()
        l_ProjectStatusAutomationLink : list[zou.app.models.project.ProjectStatusAutomationLink] = zou.app.models.project.ProjectStatusAutomationLink.query.all()
        l_ProjectAssetTypeLink : list[zou.app.models.project.ProjectAssetTypeLink] = zou.app.models.project.ProjectAssetTypeLink.query.all()
        l_ProjectTaskStatusLink : list[zou.app.models.project.ProjectTaskStatusLink] = zou.app.models.project.ProjectTaskStatusLink.query.all()
        l_ProjectTaskTypeLink : list[zou.app.models.project.ProjectTaskTypeLink] = zou.app.models.project.ProjectTaskTypeLink.query.all()
        l_ProjectPersonLink : list[zou.app.models.project.ProjectPersonLink] = zou.app.models.project.ProjectPersonLink.query.all()
        l_Entity : list[zou.app.models.entity.Entity] = zou.app.models.entity.Entity.query.all()
        l_AssetInstanceLink : list[zou.app.models.entity.AssetInstanceLink] = zou.app.models.entity.AssetInstanceLink.query.all()
        l_EntityLink : list[zou.app.models.entity.EntityLink] = zou.app.models.entity.EntityLink.query.all()
        l_EntityConceptLink : list[zou.app.models.entity.EntityConceptLink] = zou.app.models.entity.EntityConceptLink.query.all()
        l_EntityVersion : list[zou.app.models.entity.EntityVersion] = zou.app.models.entity.EntityVersion.query.all()
        l_Notification : list[zou.app.models.notification.Notification] = zou.app.models.notification.Notification.query.all()
        l_subscription : list[zou.app.models.subscription.Subscription] = zou.app.models.subscription.Subscription.query.all()
        with Session(engine) as session:
            doodle.base.BaseMixin.metadata.create_all(engine)
            session.add_all([doodle.studio.Studio().from_zou(i) for i in l_studio])
            session.add_all([doodle.department.Department().from_zou(i) for i in l_department])
            session.add_all([doodle.preview_background_file.PreviewBackgroundFile().from_zou(i) for i in l_preview_background_file])
            session.add_all([doodle.status_automation.StatusAutomation().from_zou(i) for i in l_status_automation])
            session.add_all([doodle.project_status.ProjectStatus().from_zou(i) for i in l_prj_list])
            session.add_all([doodle.organisation.Organisation().from_zou(i) for i in l_organisation])

            session.add_all([doodle.task_status.TaskStatus().from_zou(i) for i in l_task_status])
            session.add_all([doodle.task_type.TaskType().from_zou(i) for i in l_task_type])
            session.add_all([doodle.entity_type.EntityType().from_zou(i) for i in l_entity_type])
            session.add_all([doodle.entity_type.TaskTypeAssetTypeLink().from_zou(i) for i in l_TaskTypeAssetTypeLink])
            session.add_all([doodle.person.Person().from_zou(i) for i in l_person])
            session.add_all([doodle.person.DepartmentLink().from_zou(i) for i in l_DepartmentLink])
            session.add_all([doodle.metadata_descriptor.MetadataDescriptor().from_zou(i) for i in l_metadata_descriptor])
            session.add_all([doodle.metadata_descriptor.DepartmentMetadataDescriptorLink().from_zou(i) for i in l_DepartmentMetadataDescriptorLink])
            session.add_all([doodle.project.Project().from_zou(i) for i in l_project])
            session.add_all([doodle.project.ProjectPreviewBackgroundFileLink().from_zou(i) for i in l_ProjectPreviewBackgroundFileLink])
            session.add_all([doodle.project.ProjectStatusAutomationLink().from_zou(i) for i in l_ProjectStatusAutomationLink])
            session.add_all([doodle.project.ProjectAssetTypeLink().from_zou(i) for i in l_ProjectAssetTypeLink])
            session.add_all([doodle.project.ProjectTaskStatusLink().from_zou(i) for i in l_ProjectTaskStatusLink])
            session.add_all([doodle.project.ProjectTaskTypeLink().from_zou(i) for i in l_ProjectTaskTypeLink])
            session.add_all([doodle.project.ProjectPersonLink().from_zou(i) for i in l_ProjectPersonLink])

            # l_tmp = [doodle.entity.Entity().from_zou(i) for i in l_Entity]
            # l_tmp2 = [x for x in l_tmp if x.first_name == "朱丽飞"]
            # print(l_tmp)
            
            
            session.add_all([doodle.entity.Entity().from_zou(i) for i in l_Entity])
            session.add_all([doodle.entity.AssetInstanceLink().from_zou(i) for i in l_AssetInstanceLink])
            session.add_all([doodle.entity.EntityLink().from_zou(i) for i in l_EntityLink])
            session.add_all([doodle.entity.EntityConceptLink().from_zou(i) for i in l_EntityConceptLink])
            session.add_all([doodle.entity.EntityVersion().from_zou(i) for i in l_EntityVersion])
            session.add_all([doodle.notification.Notification().from_zou(i) for i in l_Notification])
            session.add_all([doodle.subscription.Subscription().from_zou(i) for i in l_subscription])

            session.commit()



if __name__ == "__main__":
    if(os.path.exists(f"D:\\{DB_NAME}")):
        os.remove(f"D:\\{DB_NAME}")
    main()
